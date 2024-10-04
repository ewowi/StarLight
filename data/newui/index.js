// @title     StarBase
// @file      newui.js
// @date      20240819
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

class Controller {

  //kept vars public as other classes uses them
  ws = null
  sysInfo = {}

  modules = null
  theme = null
  mainNav = null

  onLoad() {
    document.title = appName() + "💫 by MoonModules 🌔"
    if (window.location.href.includes("127.0.0.1"))
      this.fetchModelForLiveServer();
    else 
      this.makeWS();

    this.modules = new Modules();

    this.theme = new Theme();
    this.theme.createHTML();
    this.theme.getTheme();
  
    this.mainNav = new MainNav(this.modules.model);
    this.mainNav.createHTML();

    userFunSetup()
  }

  async fetchModelForLiveServer() {
    // Mock fetch for testing while using Live Server. No error checking for brevity.
    // Replace with call to server websocket
    let localModel = await (await fetch('/misc/model.json')).json()
  
    //sort modules by order (o)
    localModel.sort(function(a,b) {
      return Math.abs(a.o) - Math.abs(b.o); //o is order nr (ignore negatives for the time being)
    });
  
    for (let moduleJson of localModel) {
      this.modules.addModule(moduleJson)
    }

    //send sysInfo
    let json = {}
    json.sysInfo = {};
    json.sysInfo.board = "esp32"
    json.sysInfo.nrOfPins = 40
    json.sysInfo.pinTypes = []
    for (let pinNr = 0; pinNr < json.sysInfo.nrOfPins; pinNr++)
      json.sysInfo.pinTypes[pinNr] = Math.round(Math.random() * 3)
    this.receiveData(json);

    // every 1 second
    window.setInterval(function() {
      controller.modules.generateData() //generates data for each variabe in model
    }, 1000);

    // every 10th second send binarydata
    window.setInterval(function(){
      let buffer = [0,1,2,3,4]
      buffer[0] = Math.round(Math.random())
      
      // console.log(buffer)
      // let buffer = new Uint8Array([0,1,2,3,4,5,6,7,8]);
      if (buffer[0] == 0) {
        let canvasNode = gId("Pins.board");
        if (canvasNode) {
          // console.log(buffer, previewNode);
          for (let pinNr = 0; pinNr < controller.sysInfo.nrOfPins; pinNr++)
            buffer[pinNr+5] = Math.round(Math.random() * 256)
          controller.modules.previewBoard(canvasNode, buffer);
        }
      }
      else if (buffer[0] == 1) {
        //LEDs specific
        let canvasNode = gId("Fixture.preview");
        if (canvasNode) {
          // console.log(buffer, canvasNode);
          let previewVar = controller.modules.findVar("Fixture", "preview");
          if (previewVar.file) {
            buffer[1] = 0
            buffer[2] = 0
            buffer[3] = 0
            for (let i = 0; i < previewVar.file.nrOfLeds; i++) {
              buffer[4+i*3] = Math.random() * 256
              buffer[4+i*3+1] = Math.random() * 256
              buffer[4+i*3+2] = Math.random() * 256
            }

            userFun(buffer);
          }
        }
        //end LEDs specific

      }
    }, 100);

  }

  makeWS() {
    if (this.ws) return;
    let url = (window.location.protocol == "https:"?"wss":"ws")+'://'+window.location.hostname+'/ws';
    console.log("makeWS url", url);
    this.ws = new WebSocket(url);
    this.ws.binaryType = "arraybuffer";
    this.ws.onmessage = (e)=>{
      if (e.data instanceof ArrayBuffer) { // binary packet - e.g. for preview
        let buffer = new Uint8Array(e.data);
        if (buffer[0]==0) {
          let canvasNode = gId("Pins.board");
          if (canvasNode) {
            // console.log(buffer, canvasNode);
            this.modules.previewBoard(canvasNode, buffer);
          }
        }
        else {
          userFun(buffer);
        }
      } 
      else {
        // console.log("onmessage", e.data);
        let json = null;
        try {
          json = JSON.parse(e.data);
        } catch (error) {
            json = null;
            console.error("makeWS json error", error, e.data); // error in the above string (in this case, yes)!
        }
        if (json) {
          //receive model per module to stay under websocket size limit of 8192
          if (json.type && ["appmod","usermod", "sysmod"].includes(json.type)) { //generate array of variables
            let found = false;
            for (let module of this.modules.model) {
              if (module.id == json.id)
                found = true;
            }
            if (!found) {
              this.modules.addModule(json);
            }
            else
              console.log("html of module already generated", json);
          }
          else { //update
            if (!Array.isArray(json)) {//only the model is an array
              // console.log("WS receive update", json);
              this.receiveData(json);
            }
            else
              console.log("dev array not expected", json);
          }
        }
      }
    }
    this.ws.onclose = (e)=>{
      console.log("WS close and retry", e);
      setTimeout(makeWS, 1500); // retry WS connection
      this.ws = null;
    }
    this.ws.onopen = (e)=>{
      console.log("WS open", e);
    }
    this.ws.onerror = (e)=>{
      console.log("WS error", e);
    }
  } // makeWS

  requestJson(command) {
    if (window.location.href.includes("127.0.0.1")) { //Live Server
      // after a short delay (ws roundtrip)
      // Live server generates data every second
    } else {
      if (!this.ws) return;

      let req = JSON.stringify(command);
      
      // console.log("requestJson", command);
        
      this.ws.send(req);
    }
  }

  receiveData(json) {
    // console.log("receiveData", json)
    if (isObject(json)) {
      for (let key of Object.keys(json)) {
        let value = json[key] //contains pid.id
        let pidid = key.split(".")
        let variable = this.modules.findVar(pidid[0], pidid[1]);       
        if (variable) {
          let variableClass = varJsonToClass(variable);
          variableClass.receiveData(value)
        } // if variable
        else if (key == "sysInfo") { //update the row of a table
          // ppf("receiveData", key, value.board);
          this.sysInfo = value;
        }
      } //for key
    }
  }
  
} //class Controller

/**
 * Create an instance of the app on the global space
 */
window.controller = new Controller()






// Utility functions

const UINT8_MAX = 255;
const UINT16_MAX = 256*256-1;

function gId(c) {return document.getElementById(c);}
function cE(e) { return document.createElement(e); }

function initCap(s) {
  if (typeof s !== 'string') return '';
  // https://www.freecodecamp.org/news/how-to-capitalize-words-in-javascript/
  return s.replace(/[\W_]/g,' ').replace(/(^\w{1})|(\s+\w{1})/g, l=>l.toUpperCase()); // replace - and _ with space, capitalize every 1st letter
}

//https://stackoverflow.com/questions/8511281/check-if-a-value-is-an-object-in-javascript
function isObject(val) {
  if (Array.isArray(val)) return false;
  if (val === null) { return false;}
  return ( (typeof val === 'function') || (typeof val === 'object'));

  //or   return obj === Object(obj); //true for arrays.???
}

//WLEDMM: utility function to load contents of file from FS (used in draw)
function fetchAndExecute(url, name, parms, callback, callError = null)
{
  fetch
  (url+name, {
    method: 'get'
  })
  .then(res => {
    if (!res.ok) {
      if (callError) callError("File " + name + " not found");
    	return "";
    }
    // console.log("res", res, res.text(), res.text().result);
    return res.text();
  })
  .then(text => {
    // console.log("text", text);
    callback(parms, text);
  })
  .catch(function (error) {
	if (callError) callError(parms, "Error getting " + name);
	console.log(error);
  })
  .finally(() => {
    // if (callback) setTimeout(callback,99);
  });
}