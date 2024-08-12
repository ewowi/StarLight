// @title     StarBase
// @file      newui.js
// @date      20240720
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

let ws = null;
let mainNav; //stores MainNav class instance at onLoad
let model = []; //model.json (as send by the server), used by FindVar

function onLoad() {
  mainNav = new MainNav();
  mainNav.addBody();

  if (window.location.href.includes("127.0.0.1"))
    fetchModelForLiveServer();
  else 
    makeWS();
}

async function fetchModelForLiveServer() {
  // Mock fetch for testing while using Live Server. No error checking for brevity.
  // Replace with call to server websocket
  model = await (await fetch('../misc/model.json')).json()

  for (let module of model) {
    addModule(module)
  }
}

function makeWS() {
  if (ws) return;
  let url = (window.location.protocol == "https:"?"wss":"ws")+'://'+window.location.hostname+'/ws';
  console.log("makeWS url", url);
  ws = new WebSocket(url);
  ws.binaryType = "arraybuffer";
  ws.onmessage = (e)=>{
    if (e.data instanceof ArrayBuffer) { // binary packet - e.g. for preview
      let buffer = new Uint8Array(e.data);
      if (buffer[0]==0) {
        let pviewNode = gId("board");
        // console.log(buffer, pviewNode);
        if (pviewNode)
          previewBoard(pviewNode, buffer);
      }
      else 
        userFun(buffer);
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
          for (let module of model) {
            if (module.id == json.id)
              found = true;
          }
          if (!found) {
            model.push((json)); //this is the model
            addModule(json);
          }
          else
            console.log("html of module already generated", json);
        }
        else { //update
          if (!Array.isArray(json)) {//only the model is an array
            // console.log("WS receive update", json);
            // receiveData(json);
          }
          else
            console.log("dev array not expected", json);
        }
      }
    }
  }
  ws.onclose = (e)=>{
    console.log("WS close and retry", e);
    gId('connind').style.backgroundColor = "var(--c-r)";
    setTimeout(makeWS,1500); // retry WS connection
    ws = null;
  }
  ws.onopen = (e)=>{
    console.log("WS open", e);
  }
  ws.onerror = (e)=>{
    console.log("WS error", e);
  }
}

//used by fetchModel and by makeWS
function addModule(module) {
  // let module = json;
  console.log("WS receive module", module);

  //updateUI is made after all modules have been fetched, how to adapt to add one module?
  mainNav.updateUI(function(activeModule) {
    let code = "";
    for (variable of activeModule.n) {
      console.log(variable);
      code += `<p>`
      code += `<label>${variable.id}</label>`
      code += `<input type=${variable.type}></input>`
      code += `<p>`
    }
    code += '<pre>' + JSON.stringify(activeModule, null, 2) + '</pre>'
    return code
  });

}