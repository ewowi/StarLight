// @title     StarBase
// @file      index.css
// @date      20240411
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

let d = document;
let ws = null;

let nrOfMdlColumns = 4;
let jsonValues = {};
let onUICommands = [];
let model = []; //model.json (as send by the server), used by FindVar
let savedView = null;

//C++ equivalents
const UINT8_MAX = 255;
const UINT16_MAX = 256*256-1;

const pinTypeIO = 0;
const pinTypeReadOnly = 1;
const pinTypeReserved = 2;
const pinTypeSpi = 3;
const pinTypeInvalid = UINT8_MAX;
let sysInfo = {};
function getPinType(pinNr) {
  if (sysInfo.pinTypes[pinNr] == pinTypeIO) return "🟢";
  else if (sysInfo.pinTypes[pinNr] == pinTypeReadOnly) return "🟠";
  else if (sysInfo.pinTypes[pinNr] == pinTypeReserved) return "🟣";
  else return "🔴";
}

function gId(c) {return d.getElementById(c);}
function cE(e) { return d.createElement(e); }

function handleVisibilityChange() {
  console.log("handleVisibilityChange");
}

function onLoad() {
  getTheme();

  makeWS();

  initMdlColumns();

  d.addEventListener("visibilitychange", handleVisibilityChange, false);
}

function ppf() {
  let logNode = gId("log");
  let sep = "";
  if (logNode) {
    if (logNode.value.length > 64000) logNode.value = ""; //reset if too big
    // console.log("conslog", theArgs);
    for (var i = 0; i < arguments.length; i++) {
      // console.log(arguments[i]);
      if (Object.keys(arguments[i]))
        logNode.value += sep + JSON.stringify(arguments[i]);
      else
        logNode.value += sep + arguments[i];
      sep = " ";
      // "WS receive createHTML " + module.id + "\n";
    }
    logNode.value += "\n";
    // for (const arg of theArgs) {
    //   console.log(arg);
    // }
    logNode.scrollTop = logNode.scrollHeight;
  }
  else 
    console.log(arguments);
}

function makeWS() {
  if (ws) return;
  let url = (window.location.protocol == "https:"?"wss":"ws")+'://'+window.location.hostname+'/ws';
  console.log("makeWS url", url);
  ws = new WebSocket(url);
  ws.binaryType = "arraybuffer";
  ws.onmessage = (e)=>{
    if (e.data instanceof ArrayBuffer) { // preview packet
      let buffer = new Uint8Array(e.data);
      if (buffer[0]==0) {
        let pviewNode = gId("board");
        // console.log(buffer, pviewNode);
        previewBoard(pviewNode, buffer);
      }
      else 
        userFun(buffer);
    } 
    else {
      // console.log("onmessage", e.data);
      clearTimeout(jsonTimeout);
      jsonTimeout = null;
      gId('connind').style.backgroundColor = "var(--c-l)";

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
            let module = json;
            model.push((module)); //this is the model
            console.log("WS receive createHTML", module);
            ppf("WS receive createHTML", module.id);
            createHTML(module); //no parentNode

            if (module.id == "System") {
              console.log("system changes", module);
              if (module.view)
                savedView = module.view;
              if (module.theme)
                changeHTMLTheme(module.theme);
            }

            //rerun after each module added
            if (window.location.href.includes("4.3.2.1")) //captive portal
              changeHTMLView("vSetup"); //captive portal shows setup screen
            else if (savedView)
              changeHTMLView(savedView);
            else
              changeHTMLView("vApp"); //default

            gId("vApp").value = appName(); //tbd: should be set by server

            //send request for onUI
            flushOnUICommands();
          }
          else
            console.log("html of module already generated", json);
        }
        else { //update
          if (!Array.isArray(json)) //only the model is an array
            // console.log("WS receive update", json);
            receiveData(json);
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
		reqsLegal = true;
  }
  ws.onerror = (e)=>{
    console.log("WS error", e);
  }
}

function linearToLogarithm(json, value) {
  if (value == 0) return 0;

  var minp = json.min?json.min:0;
  var maxp = json.max?json.max:255;

  // The result should be between 100 an 10000000
  var minv = minp?Math.log(minp):0;
  var maxv = Math.log(maxp);

  // calculate adjustment factor
  var scale = (maxv-minv) / (maxp - minp);

  let result = Math.exp(minv + scale*(value-minp));

  // console.log(json, minv, maxv, scale, result);

  return Math.round(result);
}

function createHTML(json, parentNode = null, rowNr = UINT8_MAX) {

  // console.log("createHTML", json, parentNode);
  if (Array.isArray(json)) {
    //sort according to o value
    json.sort(function(a,b) {
      return Math.abs(a.o) - Math.abs(b.o); //o is order nr (ignore negatives for the time being)
    });

    for (let variable of json) { //if isArray then variables of array
      createHTML(variable, parentNode, rowNr);
    }
  }
  else { // json is variable
    let  variable = json;

    if (Array.isArray(variable.value) && rowNr != UINT8_MAX) {
      if ((rowNr < variable.value.length && variable.value[rowNr] == null)) { //rowNr >= variable.value.length || needed if row is created but value not yet
        // console.log("not showing this var as value is null", variable, rowNr);
        return;
      }
      if (parentNode.className == "ndiv" && rowNr >= variable.value.length) {
        console.log("createHTML ndiv parent no create", parentNode, variable, rowNr, variable.value.length);
        return;
      }
    }

    //if root (type module) add the html to one of the mdlColumns
    if (parentNode == null) {
      parentNode = gId("mdlColumn" + variable.o%nrOfMdlColumns);
    }
    let parentNodeType = parentNode.nodeName.toLocaleLowerCase();

    // if (!variable || !variable.id) {
    //   console.log("genHTML no variable and id", variable, parentNode); //tbd: caused by more data then columns in table...
    //   return;
    // }

    let divNode = null; //divNode will be appended to the parentNode after if then else and returned
    let varNode = null; //the node containing the variable
    let rangeValueNode = null;
    // let buttonSaveNode = null;
    // let buttonCancelNode = null;

    let ndivNeeded = true; //for details ("n"), module and table do not need an extra div for details
       
    let labelNode = cE("label");
    let parentVar = findParentVar(variable.id);
    if (parentVar && variable.id != parentVar.id && parentVar.id && variable.id.substring(0, parentVar.id.length) == parentVar.id) { // if parent id is beginning of the name of the child id then remove that part
      labelNode.innerText = initCap(variable.id.substring(parentVar.id.length)); // the default when not overridden by onUI
    }
    else
      labelNode.innerText = initCap(variable.id); // the default when not overridden by onUI
    
    divNode = cE("div");
    divNode.id = variable.id + (rowNr != UINT8_MAX?"#" + rowNr:"") + "_d";

    //table cells and buttons don't get a label
    if (parentNodeType != "td" && variable.type != "checkbox") { //has its own label
      if (variable.type != "button" && !["appmod","usermod", "sysmod"].includes(variable.type)) divNode.appendChild(labelNode); //add label (tbd:must be done by childs n table cell)
    }

    if (["appmod", "usermod", "sysmod"].includes(variable.type)) { //if module
      ndivNeeded = false;

      varNode = cE("div");
      let mdlName = findVar("mdlName");
      if (mdlName && mdlName.value) { //sometimes value not set yet
        // console.log("createModule", variable, mdlName);
        let index = mdlName.value.indexOf(variable.id); //find this module
        if (index != -1) {
          let mdlEnabled = findVar("mdlEnabled");
          if (mdlEnabled)
            varNode.hidden = !mdlEnabled.value[index]; //hidden if not enabled
        }
      }

      let hgroupNode = cE("hgroup");

      let h2Node = cE("h2");
      h2Node.style="float: left;";
      h2Node.innerText = initCap(variable.id);
      hgroupNode.appendChild(h2Node);

      let minusNode = cE("span");
      minusNode.innerText = "🟡";
      minusNode.style="float: right;"
      minusNode.addEventListener('click', (event) => {console.log("minus click", event.target); event.target.parentNode.parentNode.parentNode.hidden = true;});
      hgroupNode.appendChild(minusNode);

      let helpNode = cE("a");
      helpNode.innerText = "ⓘ";
      helpNode.style="float: right;"
      let initCapVarType = variable.id=="Workflow"?"SysMod":variable.type=="appmod"?appName() + "Mod":variable.type=="usermod"?"UserMod":"SysMod"; 
      helpNode.setAttribute('href', "https://ewowi.github.io/StarDocs/" + initCapVarType + "/" + initCapVarType + initCap(variable.id));
      hgroupNode.appendChild(helpNode);

      varNode.appendChild(hgroupNode);

      //otherwise next node is not positioned right. Improvements welcome on this hack
      varNode.appendChild(cE("br"));
      varNode.appendChild(cE("br"));
    }
    else if (variable.type == "table") {
      ndivNeeded = false;

      //add table
      varNode = cE("table");

      let theadNode = cE("thead");
      theadNode.appendChild(cE("tr"));
      varNode.appendChild(theadNode); //row for header

      let tbodyNode = cE("tbody");
      varNode.appendChild(tbodyNode);

      if (!variable.ro && variable.id != "fileTbl") { //fileTbl has upload file
        let buttonNode = cE("input");
        buttonNode.type = "button";
        buttonNode.value = "+";
        buttonNode.addEventListener('click', (event) => {
          console.log("Table +", event.target);

          var command = {};
          command.addRow = {};
          command.addRow.id = variable.id;
          requestJson(command);
        });
        divNode.appendChild(buttonNode);
      }

      //variable.n will add the columns
    } else if (parentNodeType == "table") { 

      // console.log("tableChild", parentNode, variable);

      varNode = cE("th");
      varNode.innerText = initCap(variable.id); //label onUI response can change it

    } else if (variable.type == "select" || variable.type == "pin" || variable.type == "ip") {

      if (variable.ro) { //e.g. for reset/restart reason: do not show a select but only show the selected option
        varNode = cE("span");
      }
      else {
        varNode = cE("select");
        varNode.addEventListener('change', (event) => {console.log("select change", event);sendValue(event.target);});
       }

      if (variable.type == "pin") {
        variable.options = [];
        for (let index = 0; index < 40; index++) {
          variable.options.push(index + " " + getPinType(index));
        }
      }

    } else if (variable.type == "canvas") {

      //3 lines of code to only add 🔍
      let spanNode = cE("span");
      spanNode.innerText= "🔍";
      divNode.appendChild(spanNode);
      divNode.appendChild(cE("br"));

      varNode = cE("canvas");
      varNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});

    } else if (variable.type == "textarea") {

      //3 lines of code to only add 🔍
      let spanNode = cE("span");
      spanNode.innerText= "🔍";
      divNode.appendChild(spanNode);
      divNode.appendChild(cE("br"));

      varNode = cE("textarea");
      varNode.rows = 10;
      varNode.readOnly = variable.ro;
      varNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});
    }
    else if (variable.type == "url") {
      varNode = cE("a");
      // varNode.setAttribute('target', "_blank"); //does not work well on mobile
    } else if (variable.type == "checkbox") {
      varNode = cE("label");
      if (parentNodeType != "td") {
        let spanNode = cE("span");
        spanNode.innerText = initCap(variable.id) + " "; // the default when not overridden by onUI
        varNode.appendChild(spanNode);
      }
      let inputNode = cE("input");
      inputNode.type = variable.type;
      inputNode.disabled = variable.ro;
      inputNode.indeterminate = true; //until it gets a value;
      inputNode.addEventListener('change', (event) => {console.log(variable.type + " change", event.target.parentNode);sendValue(event.target.parentNode);}); //send the label
      varNode.appendChild(inputNode);
      let cmarkNode = cE("span");
      cmarkNode.className = "checkmark";
      varNode.appendChild(cmarkNode);
    } else if (variable.type == "button") {
      varNode = cE("input");
      varNode.type = variable.type;
      varNode.disabled = variable.ro;
      varNode.value = initCap(variable.id); //initial label, button.value is the label shown on the button
      varNode.addEventListener('click', (event) => {console.log(variable.type + " click", event.target);sendValue(event.target);});
    } else if (variable.type == "range") {
      varNode = cE("input");
      varNode.type = variable.type;
      varNode.min = variable.min?variable.min:0;
      varNode.max = variable.max?variable.max:255; //range slider default 0..255
      varNode.disabled = variable.ro;
      //numerical ui value changes while draging the slider (oninput)
      let rvNode = variable.id + (rowNr != UINT8_MAX?"#" + rowNr:"") + "_rv";
      varNode.addEventListener('input', (event) => {
        if (gId(rvNode)) {
          gId(rvNode).innerText = variable.log?linearToLogarithm(variable, event.target.value):event.target.value;
        }
      });
      //server value changes after draging the slider (onchange)
      varNode.addEventListener('change', (event) => {
        sendValue(event.target);
      });
      rangeValueNode = cE("span");
      rangeValueNode.id = rvNode; //rangeValue
    } else if (variable.type == "coord3D") {
      varNode = cE("span");
      if (!variable.ro) { //e.g. for reset/restart reason: do not show a select but only show the selected option
        let xNode = cE("input");
        xNode.type = "number";
        xNode.min = variable.min?variable.min:0; //if not specified then unsigned value (min=0)
        if (variable.max) xNode.max = variable.max;
        xNode.placeholder = "x";
        xNode.min = variable.min?variable.min:0; //if not specified then unsigned value (min=0)
        if (variable.max) xNode.max = variable.max;
        xNode.addEventListener('change', (event) => {console.log(variable.type + " change", event.target.parentNode);sendValue(event.target.parentNode);});
        varNode.appendChild(xNode);

        let yNode = xNode.cloneNode();
        yNode.placeholder = "y";
        yNode.addEventListener('change', (event) => {console.log(variable.type + " change", event.target.parentNode);sendValue(event.target.parentNode);});
        varNode.appendChild(yNode);

        let zNode = xNode.cloneNode();
        zNode.placeholder = "z";
        zNode.addEventListener('change', (event) => {console.log(variable.type + " change", event.target.parentNode);sendValue(event.target.parentNode);});
        varNode.appendChild(zNode);
      }
    } else if (variable.type == "progress") {
      varNode = cE("progress");
      varNode.min = variable.min?variable.min:0; //if not specified then unsigned value (min=0)
      if (variable.max) varNode.max = variable.max;
    } else if (variable.type == "file") {
      //https://github.com/smford/esp32-asyncwebserver-fileupload-example/blob/master/example-01/example-01.ino

      varNode = cE("span");

      inputNode = cE("input");
      inputNode.type = variable.type;
      inputNode.addEventListener('change', (event) => {
        let fileNode = event.target;
        let file = fileNode.files[0];
        let formData = new FormData();
        console.log("file " + variable.id, file, formData, file.size);
        fileNode.parentNode.querySelector("progress").max = Math.round(file.size / 10000); //set progress max in blocks of 10K
             
        formData.append("file", file);
        fetch('/' + variable.id, {method: "POST", body: formData});
      });
      varNode.appendChild(inputNode);

      let progressNode = cE("progress");
      // if (variable.max) progressNode.max = variable.max;
      progressNode.hidden = true;
      varNode.appendChild(progressNode);

      let spanNode = cE("span"); //fail or success
      varNode.appendChild(spanNode);
    } else {
      //input types: text, search, tel, url, email, and password.

      if (variable.ro && variable.type != "button") {
        varNode = cE("span");
      } else {
        varNode = cE("input");
        varNode.type = variable.type;
        varNode.addEventListener('change', (event) => {console.log(variable.type + " change", event);sendValue(event.target);});
        // if (["text", "password", "number"].includes(variable.type) ) {
        //   buttonSaveNode = cE("text");
        //   buttonSaveNode.innerText = "✅";
        //   buttonSaveNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);});
        //   buttonCancelNode = cE("text");
        //   buttonCancelNode.innerText = "🛑";
        //   buttonCancelNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);});
        // }
        if (variable.type == "number") {
          varNode.min = variable.min?variable.min:0; //if not specified then unsigned value (min=0)
          if (variable.max) varNode.max = variable.max;
        }
        else {
          if (variable.max) varNode.setAttribute('maxlength', variable.max); //for text and textarea set max length varNode.maxlength is not working for some reason
        }
      }
    } //if variable type

    if (parentNodeType == "table") { //table headers don't have a divNode (why not...)
      // divNode = varNode;
      parentNode.querySelector('thead').querySelector("tr").appendChild(varNode); //<thead><tr> (containing th)
    } else {
      divNode.appendChild(varNode);
      parentNode.appendChild(divNode);
    }
    varNode.id = variable.id + (rowNr != UINT8_MAX?"#" + rowNr:"");
    varNode.className = variable.type;

    if (rangeValueNode) divNode.appendChild(rangeValueNode); //_rv value of range / sliders
    // if (buttonSaveNode) divNode.appendChild(buttonSaveNode);
    // if (buttonCancelNode) divNode.appendChild(buttonCancelNode);
    
    //disable drag of parent module
    if (["appmod", "usermod", "sysmod"].includes(variable.type)) {
      setupModule(varNode.parentNode); //enable drag and drop of (the div of modules
      varNode.parentNode.draggable = true; //div of module
      // varNode.parentNode.addEventListener('dragstart', (event) => {event.preventDefault(); event.stopPropagation();});
    }

    if (variable.n && parentNodeType != "table") { //multiple details, not for table header
      //add a div with _n extension and details have this as parent
      if (ndivNeeded) {
        let ndivNode = cE("div");
        ndivNode.id = variable.id + (rowNr != UINT8_MAX?"#" + rowNr:"") + "_n";
        ndivNode.className = "ndiv";
        divNode.appendChild(ndivNode); // add to the parent of the node
        createHTML(variable.n, ndivNode, rowNr);
      }
      else
        createHTML(variable.n, varNode, rowNr); //details (e.g. module)
    }

    //don't call onUI on table rows (the table header calls onUI and propagate this to table row columns in changeHTML when needed - e.g. select)
    if (variable.fun == null || variable.fun == -2) { //request processed
      variable.chk = "gen2";
      changeHTML(variable, variable, rowNr); // set the variable with its own changed values
    }
    else { //onUI
      if (variable.value)
        changeHTML(variable, {"value":variable.value, "chk":"gen1"}, rowNr); //set only the value

      if (variable.options) // eg for pin type
        changeHTML(variable, {"options":variable.options, "chk":"gen1"}, rowNr); //set only the options

      //call ui Functionality, if defined (to set label, comment, select etc)
      if (variable.fun >= 0) { //>=0 as element in var
        onUICommands.push(variable.id);
        if (onUICommands.length > 4) { //every 4 vars (to respect responseDoc size) check WS_EVT_DATA info
          flushOnUICommands();
        }
        variable.fun = -1; //requested
      }
    }

    return varNode;
  } //not an array but variable
}

function genTableRowHTML(json, parentNode = null, rowNr = UINT8_MAX) {
  let variable = json;
  let tbodyNode = parentNode.querySelector("tbody");
  // console.log("genTableRowHTML", variable, parentNode.id, rowNr, tbodyNode.querySelectorAll("tr").length);

  //create a new row on the table
  let trNode = cE("tr");
  tbodyNode.appendChild(trNode);
  //genHTML for var(n)
  for (let columnVar of variable.n) {
    let tdNode = cE("td");
    trNode.appendChild(tdNode);
    createHTML(columnVar, tdNode, rowNr); //will also do the values
  }
  if (!variable.ro) {
    let tdNode = cE("td");
    let buttonNode = cE("input");
    buttonNode.id = variable.id + "#" + rowNr + "_del";
    buttonNode.type = "button";
    buttonNode.value = "-";
    buttonNode.addEventListener('click', (event) => {
      console.log("Table -", event.target);

      var command = {};
      command.delRow = {};
      command.delRow.id = variable.id;
      command.delRow.rowNr = rowNr;
      requestJson(command);

    });
    tdNode.appendChild(buttonNode);
    trNode.appendChild(tdNode);
  }
  flushOnUICommands();
  if (variable.id == "insTbl")
    setInstanceTableColumns();
}

function varRemoveValuesForRow(variable, rowNr) {
  if (variable.n) {
    for (let childVar of variable.n) {
      if (Array.isArray(childVar.value)) {
        childVar.value.splice(rowNr);
      }
      varRemoveValuesForRow(childVar, rowNr);
    }
  }
}

//process json from server, json is assumed to be an object
function receiveData(json) {
  // console.log("receiveData", json);

  if (isObject(json)) {
    for (let key of Object.keys(json)) {
      let value = json[key];

      //tbd: for each node of a variable (rowNr)

      //special commands
      if (key == "onUI") {
        ppf("receiveData no action", key, value); //should not happen anymore
      }
      else if (key == "view") {
        ppf("receiveData", key, value);
        changeHTMLView(value);
      }
      else if (key == "theme") {
        ppf("receiveData", key, value);
        changeHTMLTheme(value);
      }
      else if (key == "canvasData") {
        ppf("receiveData no action", key, value);
      } else if (key == "details") {
        let variable = value.var;
        let rowNr = value.rowNr == null?UINT8_MAX:value.rowNr;
        let nodeId = variable.id + ((rowNr != UINT8_MAX)?"#" + rowNr:"");
        //if var object with .n, create .n (e.g. see fx.onChange (setEffect) and fixtureGenonChange, tbd: )
        ppf("receiveData details", key, variable.id, nodeId, rowNr);
        if (gId(nodeId + "_n")) gId(nodeId + "_n").remove(); //remove old ndiv

        let modelVar = findVar(variable.id);
        modelVar.n = variable.n;

        //create new ndiv
        if (modelVar.n) {
          let ndivNode = cE("div");
          ndivNode.id = nodeId + "_n";
          ndivNode.className = "ndiv";
          gId(nodeId).parentNode.appendChild(ndivNode);
          createHTML(modelVar.n, ndivNode, rowNr);
        }
        flushOnUICommands(); //make sure onUIs of new elements are called
      }
      else if (key == "addRow") { //update the row of a table
        ppf("receiveData", key, value);

        if (value.id && value.rowNr != null) {
          let tableId = value.id;
          let rowNr = value.rowNr;

          let tableVar = findVar(tableId);
          let tableNode = gId(tableId);
          let tbodyNode = tableNode.querySelector("tbody");

          ppf("addRow ", tableVar, tableNode, rowNr);

          let newRowNr = tbodyNode.querySelectorAll("tr").length;

          genTableRowHTML(tableVar, tableNode, newRowNr);
        }
        else 
          ppf("dev receiveData addRow no id and/or rowNr specified", key, value);

      } else if (key == "delRow") { //update the row of a table

        ppf("receiveData", key, value);
        let tableId = value.id;
        let tableVar = findVar(tableId);
        let rowNr = value.rowNr;

        //delete the row here as well...

        let tableNode = gId(tableId);

        tableNode.deleteRow(rowNr + 1); //header counts as 0

        varRemoveValuesForRow(tableVar, rowNr);

        ppf("delRow ", tableVar, tableNode, rowNr);

      } else if (key == "updRow") { //update the row of a table

        let tableId = value.id;
        let tableVar = findVar(tableId);
        let rowNr = value.rowNr;
        let tableRow = value.value;

        // ppf("receiveData updRow", key, tableId, rowNr, tableRow);
        // ppf("updRow main", tableId, tableRows, tableNode, tableVar);

        let colNr = 0;
        for (let colVar of tableVar.n) {
          let colValue = tableRow[colNr];
          // ppf("    col", colNr, colVar, colValue);
          changeHTML(colVar, {"value":colValue, "chk":"updRow"}, rowNr);
          colNr++;
        }

      } else if (key == "sysInfo") { //update the row of a table
        ppf("receiveData", key, value.board);
        sysInfo = value;
      } else { //{variable:{label:value:options:comment:}}

        let variable = findVar(key);

        if (variable) {
          let rowNr = value.rowNr == null?UINT8_MAX:value.rowNr;
          // if (variable.id == "ledsEnd" || variable.id == "ledsSize" || variable.id == "point")
          //   ppf("receiveData ", variable, value);
          variable.fun = -2; // request processed

          value.chk = "onUI";
          changeHTML(variable, value, rowNr); //changeHTML will find the rownumbers if needed
        }
        else
          ppf("receiveData key is no variable", key, value);
      }
    } //for keys
  } //isObject
  else
    ppf("receiveData no Object", object);
} //receiveData

//do something with an existing (variable) node, key is an existing node, json is what to do with it
function changeHTML(variable, commandJson, rowNr = UINT8_MAX) {

  let node = null;

  if (rowNr != UINT8_MAX) node = gId(variable.id + "#" + rowNr);
  else node = gId(variable.id);

  if (!node) {
    //we should find all nodes
    let rowNodes = document.querySelectorAll(`${variable.type}[id^="${variable.id}#"]`); //find nodes from variable.type class with id + #nr (^: starting with)
    for (let subNode of rowNodes) {
      let rowNr = parseInt(subNode.id.substring(variable.id.length + 1));
      // console.log("changeHTML found row nodes !", variable.id, subNode.id, commandJson, rowNr);
      changeHTML(variable, commandJson, rowNr); //recursive call of all nodes
    }
    // if (rowNodes.length == 0) //can happen e.g. fixture parameters
    //   console.log("changeHTML no node !", variable, node, commandJson, rowNr);
    return;
  }

  let nodeType = node.nodeName.toLocaleLowerCase();

  if (commandJson.hasOwnProperty("label")) {
    if (nodeType == "th") //table header
      node.innerText = initCap(commandJson.label);
    else if (node.className == "button") {
      node.value = initCap(commandJson.label);
    }
    else if (node.className == "checkbox") {
      node.querySelector("span").innerText = initCap(commandJson.label) + " ";
    }
    else {
      let labelNode = node.parentNode.querySelector("label");
      if (labelNode) labelNode.innerText = initCap(commandJson.label);
    }

    variable.label = commandJson.label;
  } //label

  if (commandJson.hasOwnProperty("comment")) {
    
    if (nodeType != "th") {

      //if not a tablecell
      if (node.parentNode.parentNode.nodeName.toLocaleLowerCase() != "td") {
        let commentNode; // = node.parentNode.querySelector('comment');
        for (let childNode of node.parentNode.childNodes) {
          if (childNode.nodeName.toLocaleLowerCase() == "comment") {
            commentNode = childNode;
            break;
          }
        }
        // console.log("commentNode", commentNode);
        if (!commentNode) { //create if not exist
          commentNode = cE("comment");
          node.parentNode.appendChild(commentNode);
        }
        commentNode.innerText = commandJson.comment;
      }
    }
    else { //th
      // console.log("changeHTML comment", variable, node, commandJson, rowNr);
      let ttdivNode = cE("div");
      ttdivNode.innerText = node.innerText;
      node.innerText = "";
      ttdivNode.classList.add("tooltip");
      let spanNode = cE("span");
      spanNode.innerHTML = commandJson.comment;
      spanNode.classList.add("tooltiptext");
      ttdivNode.appendChild(spanNode);

      node.appendChild(ttdivNode);

    }
    variable.comment = commandJson.comment;
  } //comment

  if (commandJson.hasOwnProperty("options")) { //replace the body of a table
    
    let selectNodes = [];
    //check if there are also column select cells which also needs to be updated
    if (nodeType == "th") {
      let tableNode = node.parentNode.parentNode.parentNode;
      selectNodes = tableNode.querySelector('tbody').querySelectorAll(`select[id*="${variable.id}"]`);
    }
    else if (nodeType == "select") { //span/ro will be set in .value
      selectNodes.push(node);
    }

    // console.log("commandJson.options", variable.id, commandJson.options);
    for (let selectNode of selectNodes) {
      //remove all old options first

      //remove options
      for (var i = selectNode.length-1; i > 0; i--) {
        selectNode.options[i] = null;
      }
      // remove the optgroups and their children if still exists
      var optgroup=selectNode.getElementsByTagName('optgroup')
      for (var i=optgroup.length-1;i>=0;i--) selectNode.removeChild(optgroup[i])

      var index = 0;

      //go recursively through the objects
      function findOptions(group, option, depth = 0) {
        if (isObject(option)) {
          for (let key of Object.keys(option)) { // for each group
            if (depth > 0) {
              let optNode = cE("option"); // no optgroups in optgroups so create on same level
              optNode.label = key;
              group.appendChild(optNode);
              findOptions(group, option[key], depth+1);
            }
            else {
              let optGroup = cE("optgroup");
              optGroup.label = key;
              group.appendChild(optGroup);
              findOptions(optGroup, option[key], depth+1);
            }
          }
        }
        else if (Array.isArray(option)) {
          for (let subOption of option) {
            if (Array.isArray(subOption)) { //array of arrays:  // for key values like ip nr and name
              let optNode = cE("option");
              optNode.value = subOption[0];
              optNode.text = subOption[1];
              selectNode.appendChild(optNode);
            }
            else
              findOptions(group, subOption, depth); //recursive call were subOption is object
          }
        }
        else {
          let optNode = cE("option");
          optNode.value = index++;
          optNode.text = "---------".substring(0, depth-1) + option;
          group.appendChild(optNode);
        }
      }
      findOptions(selectNode, commandJson.options); //recursively go through all the options

      // console.log("Select options", selectNode, selectNode.options);

    }
      
    variable.options = commandJson.options;

    //if no new value, set the old one
    if (commandJson.value == null)
      changeHTML(variable, {"value":variable.value, "chk":"options"}, rowNr); //(re)set the select value
    // else
    //   console.log("changeHTML value will be set in value", variable, node, commandJson, rowNr);

  } //options

  if (commandJson.hasOwnProperty("value")) { 
    //hasOwnProperty needed to catch also boolean commandJson.value when it is false !!!!
    
    // if (node.id=="insName#0" || node.id=="fx")// || node.id=="mdlEnabled" || node.id=="clIsFull" || node.id=="pin2")
    if (nodeType == "table") {
      if (Array.isArray(commandJson.value)) {
        console.log("changeHTML value table", variable, node, commandJson, rowNr);
        //remove table rows
        let tbodyNode = cE('tbody'); //the tbody of node will be replaced
        //replace the table body
        node.replaceChild(tbodyNode, node.querySelector("tbody")); //replace <table><tbody> by tbodyNode  //add to dom asap

        //add each row
        let newRowNr = 0;
        for (var row of commandJson.value) {
          if (row) { //not null, e.g. fixTbl sent value:[null]... tbd: check why
            genTableRowHTML(variable, node, newRowNr);
            let colNr = 0;
            for (let columnVar of variable.n) {
              changeHTML(columnVar, {"value": row[colNr], "chk":"table"}, newRowNr);
              colNr++;
            }
          }

          newRowNr++;
        }

        flushOnUICommands(); //make sure onUIs of new elements are called

        if (variable.id == "insTbl")
          setInstanceTableColumns();
      }
      else
        console.log("changeHTML value table no array", variable, node, commandJson, rowNr);
    }
    else if (nodeType == "th") {  //node.parentNode = table... updCol update column

      let tableNode = node.parentNode.parentNode.parentNode;
      let trNodes = tableNode.querySelector('tbody').querySelectorAll("tr");
      let tableVar = findVar(tableNode.id); //tbd: table in table
      let valueLength = Array.isArray(commandJson.value)?commandJson.value.length:1; //tbd: use table nr of rows (not saved yet)
      // console.log("changeHTML th column", node.id, (rowNr == UINT8_MAX)?JSON.stringify(commandJson.value):commandJson.value[rowNr], commandJson.chk, rowNr);

      let max = Math.max(valueLength, trNodes.length);
      for (let newRowNr = 0; newRowNr<max;newRowNr++) {
        let newValue; // if not array then use the value for each row
        if (Array.isArray(commandJson.value)) {
          newValue = commandJson.value[newRowNr];
          //hide/show disabled/enabled modules
          if (variable.id == "mdlEnabled") {
            let nameVar = findVar("mdlName");
            let mdlNode = gId(nameVar.value[newRowNr]);
            // console.log("mdlEnabled", variable, node, newValue, newRowNr, nameVar, mdlNode);
            if (mdlNode) {
              if  (mdlNode.hidden && newValue) mdlNode.hidden = false;
              if  (!mdlNode.hidden && !newValue) mdlNode.hidden = true;
            }
          }
        }
        else
          newValue = commandJson.value;

        //if row not exists, create table row
        if (newRowNr > trNodes.length - 1) {
          genTableRowHTML(tableVar, tableNode, newRowNr); //this will set the whole row and its (default) values as stored in the model
        }

        if (newRowNr < valueLength)
          changeHTML(variable, {"value":newValue, "chk":"column"}, newRowNr);
        else
          changeHTML(variable, {"value":null, "chk":"column"}, newRowNr); //new row cell has no value
      }

      flushOnUICommands(); //make sure onUIs of new elements are called

    }
    else if (node.parentNode.parentNode.nodeName.toLocaleLowerCase() == "td" && Array.isArray(commandJson.value)) { //table column, called for each column cell!!!
      // console.log("changeHTML value array", node.parentNode.parentNode.nodeName.toLocaleLowerCase(), node.id, (rowNr == UINT8_MAX)?JSON.stringify(commandJson.value):commandJson.value[rowNr], commandJson.chk, rowNr);

      if (rowNr == UINT8_MAX) {
        console.log("changeHTML value array should not happen when no rowNr", variable, node, commandJson, rowNr);
        let newRowNr = 0;
        for (let val of commandJson.value) {
          changeHTML(variable, {"value":val, "chk":"Array1"}, newRowNr); //recursive set value for variable in row
          newRowNr++;
        }
      }
      else {
        changeHTML(variable, {"value":commandJson.value[rowNr], "chk":"Array2"}, rowNr); //recursive set value for variable in row
      }
      // node.checked = commandJson.value;
    } 
    else if (node.className == "url") { //url links
      node.innerText = "🔍";
      node.setAttribute('href', commandJson.value);
    } 
    else if (node.className == "canvas")
      console.log("not called anymore");
    else if (node.className == "checkbox") {
      let value = commandJson.value;
      if (Array.isArray(commandJson.value) && rowNr != UINT8_MAX)
        value = commandJson.value[rowNr];
      node.querySelector("input").checked = value;
      node.querySelector("input").indeterminate = (value == null); //set the false if it has a non null value
    }
    else if (node.className == "button") {
      let value = commandJson.value;
      // console.log("change button", variable, node, value);
      if (Array.isArray(commandJson.value) && rowNr != UINT8_MAX) {
        value = commandJson.value[rowNr];
      }
      if (value) node.value = value; //else the id / label is used as button label
    }
    else if (node.className == "coord3D") {
      // console.log("chHTML value coord3D", node, commandJson.value, rowNr);

      if (commandJson.value) {
        //tbd: support Coord3D as array (now only objects work)
        let value = commandJson.value;
        if (Array.isArray(commandJson.value) && rowNr != UINT8_MAX)
          value = commandJson.value[rowNr];

        if (isObject(value)) { 
          if (variable.ro) {
            let sep = "";
            node.textContent = "";
            for (let key of Object.keys(value)) {
              node.textContent += sep + value[key];
              sep = ",";
            }
          }
          else {
            let index = 0;
            for (let key of Object.keys(value)) {
              let childNode = node.childNodes[index++];
              if (!childNode) console.log("dev Coord3D no child", variable, node, value, key, value[key], index);
              childNode.value = value[key];
              childNode.dispatchEvent(new Event("input")); // triggers addEventListener('input',...). now only used for input type range (slider), needed e.g. for qlc+ input
            }
          }
        }
        else 
          console.log("   dev value coord3D value not object[x,y,z]", variable.id, node.id, commandJson.value);
      }
    }
    else if (node.className == "select" || node.className == "pin" || node.className == "ip") {
      if (variable.ro) {
        var index = 0;
        if (variable.options && commandJson.value != null) { // not always the case e.g. data / table / onUI. Then value set if onUI returns
          for (var value of variable.options) {
            if (parseInt(commandJson.value) == index) {
              // console.log("changeHTML select1", value, node, node.textContent, index);
              node.textContent = value; //replace the id by its value
              // console.log("changeHTML select2", value, node, node.textContent, index);
            }
            index++;
          }
        } else
          node.textContent = commandJson.value;
      }
      else {
        if (Array.isArray(commandJson.value) && rowNr != UINT8_MAX)
          node.value = commandJson.value[rowNr];
        else
          node.value = commandJson.value;
      }
    }
    else if (node.className == "file") {
      if (variable.ro) { //text and numbers read only
        // console.log("changeHTML value span not select", variable, node, commandJson, rowNr);
      } else {
        // console.log("file update", node.id, commandJson.value);
        let inputNode = node.querySelector("input");
        let progressNode = node.querySelector("progress");
        let spanNode = node.querySelector("span");
        if (commandJson.value == UINT16_MAX - 10) {
          progressNode.hidden = true;
          spanNode.innerText = "🟢";
          inputNode.value = null;
          console.log("succes");
        }
        else if (commandJson.value == UINT16_MAX - 20) {
          progressNode.hidden = true;
          spanNode.innerText = "🔴";
          inputNode.value = null;
          console.log("fail");
        }
        else {
          spanNode.innerText = "";
          progressNode.hidden = false;
          progressNode.value = commandJson.value;
        }
        //You cannot set it to a client side disk file system path, due to security reasons.
      }
    } else if (node.className == "textarea") {
      node.value += commandJson.value;
      node.scrollTop = node.scrollHeight;
    } else {//inputs and progress type
      if (variable.ro && nodeType == "span") { //text and numbers read only
        // console.log("changeHTML value span not select", variable, node, commandJson, rowNr);
        node.textContent = commandJson.value;
      } else {
        if (Array.isArray(commandJson.value) && rowNr != UINT8_MAX)
          node.value = commandJson.value[rowNr];
        else
          node.value = commandJson.value;
        node.dispatchEvent(new Event("input")); // triggers addEventListener('input',...). now only used for input type range (slider), needed e.g. for qlc+ input
      }

      //'hack' show the instanceName on top of the page
      if (variable.id == "name") {
        gId("serverName").innerText = commandJson.value;
        document.title = commandJson.value;
      }
    }

    //value assignments depending on different situations

    if ((variable.value == null || !Array.isArray(variable.value)) && !Array.isArray(commandJson.value) && rowNr == UINT8_MAX) {
      //no arrays and rowNr. normal situation
      if (variable.value != commandJson.value)
        variable.value = commandJson.value;
    }
    else if ((variable.value == null || Array.isArray(variable.value)) && Array.isArray(commandJson.value)) {
      //both arrays
      if (rowNr) {
        if (variable.value == null) variable.value = [];
        if (variable.value[rowNr] != commandJson.value[rowNr]) {
          variable.value[rowNr] = commandJson.value[rowNr];
      }
      else {
        if (variable.value != commandJson.value)
          variable.value = commandJson.value;
      }
    }
    }
    else if ((variable.value == null || Array.isArray(variable.value)) && !Array.isArray(commandJson.value)) {
      //after changeHTML value array
      if (variable.value == null) variable.value = [];
      if (rowNr == UINT8_MAX) {
        if (variable.value != commandJson.value) {
          variable.value = commandJson.value;
        }
      } else {
        if (variable.value[rowNr] != commandJson.value) {
          variable.value[rowNr] = commandJson.value;
        }
      }
    }
    else if (!Array.isArray(variable.value) && !Array.isArray(commandJson.value) && rowNr != UINT8_MAX) {
      if (variable.value != commandJson.value) {
        console.log("chHTML column with one value for all rows", variable.id, node.id, variable.value, commandJson.value, rowNr);
        variable.value = commandJson.value; //turn variable into array
      }
    }
    else if (!Array.isArray(variable.value) && Array.isArray(commandJson.value) && rowNr == UINT8_MAX) {
      variable.value = commandJson.value; //the value turns into an array (e.g. fixtureGen parameters will become columns in panel mode)
    }
    else
      console.log("chHTML value unknown", variable.id, node.id, variable.value, commandJson.value, rowNr);

  } //value

  if (commandJson.hasOwnProperty("json")) { //json send html nodes cannot process, store in jsonValues array
    console.log("changeHTML json", variable, node, commandJson, rowNr);
    jsonValues[node.id] = commandJson.json;
    // variable[node.id].json = commandJson.json;
  }

  if (commandJson.hasOwnProperty("file")) { //json send html nodes cannot process, store in jsonValues array
    console.log("changeHTML file requested", variable.id, rowNr, commandJson);
  
    //we need to send a request which the server can handle using request variable
    let url = `http://${window.location.hostname}/file`;
    fetchAndExecute(url, commandJson.file, node.id, function(id, text) { //send node.id as parameter
      // console.log("fetchAndExecute", text); //in case of invalid commandJson
      var ledmapJson = JSON.parse(text);
      jsonValues[id] = ledmapJson;
      jsonValues[id].new = true;
      // variable[id].file = ledmapJson;
      // variable[id].file.new = true;
      console.log("changeHTML file fetched", id, ledmapJson);
    }); 
  }
} //changeHTML

function flushOnUICommands() {
  if (onUICommands.length > 0) { //if something to flush
    var command = {};
    command.onUI = onUICommands; //ask to run onUI for vars (to add the options)
    // console.log("flushOnUICommands", command);
    requestJson(command);
    onUICommands = [];
  }
}

function findVar(id, parent = model) {
  // console.log("findVar", id, parent, model);

  let foundVar = null;
  for (var variable of parent) {
    if (foundVar == null) {
      if (variable.id == id)
        foundVar = variable;
      else if (variable.n)
        foundVar = findVar(id, variable.n); //recursive
    }
  }
  return foundVar;
}

function findParentVar(id, parent = model) {
  // console.log("findParentVar", id, parent, model);

  let varArray;
  if (Array.isArray(parent))
    varArray = parent;
  else if (parent.n)
    varArray = parent.n;

  let foundVar = null;

  if (varArray) {
    for (var variable of varArray) {
      if (foundVar == null) {
        if (variable.id == id)
          foundVar = parent;
        else if (variable.n)
          foundVar = findParentVar(id, variable); //recursive
      }
    }
  }
  return foundVar;
}

var jsonTimeout;
var reqsLegal = false;

function requestJson(command) {
  gId('connind').style.backgroundColor = "var(--c-y)";
	if (command && !reqsLegal) return; // stop post requests from chrome onchange event on page restore
	if (!jsonTimeout) jsonTimeout = setTimeout(()=>{if (ws) ws.close(); ws=null; console.log("connection failed")}, 3000);

  // if (!ws) return;
  let req = JSON.stringify(command);
  
  console.log("requestJson", command);

  if (req.length > 1340)
  console.log("too big???");
  
  ws.send(req?req:'{"v":true}');

  return;
  
  let url = `http://${window.location.hostname}/json`;
  //not used at the moment as WebSockets only
  fetch(url, {
    method: 'post',
    headers: {
      "Content-type": "application/json; charset=UTF-8"
    },
    body: req
  })
  .then(res => {
    if (res) console.log("requestJson res", res, res.json());
  })
  .then(json => {
    if (json) console.log("requestJson json", json);
  })
  .catch((e)=>{
    console.log("requestJson catch", e);
  });
}

function sendValue(varNode) {
  let varId;
  if (varNode.id == "saveModel" || varNode.id == "bSave") {
    varId = "saveModel";
    gId("bSave").value = "Save";
    gId("bSave").disabled = true;
  }
  else 
  {
    varId = varNode.id;
    gId("bSave").value = "Save*";
    gId("bSave").disabled = false;
  }

  var command = {};
  command[varId] = {};
  if (varNode.className == "checkbox")
    command[varId].value = varNode.querySelector("input").checked;
  else if (varNode.className == "button") {
    // don't send the value as that is just the label
  }
  else if (varNode.className == "coord3D") {
    let coord = {};
    coord.x = parseInt(varNode.childNodes[0].value);
    coord.y = parseInt(varNode.childNodes[1].value);
    coord.z = parseInt(varNode.childNodes[2].value);
    console.log("coord", coord);
    command[varId].value = coord;
  }
  else if (varNode.nodeName.toLocaleLowerCase() == "span")
    command[varId].value = varNode.innerText;
  else //number etc
    //https://stackoverflow.com/questions/175739/how-can-i-check-if-a-string-is-a-valid-number
    command[varId].value = isNaN(varNode.value)?varNode.value:parseFloat(varNode.value); //type number is default but html converts numbers in <option> to string, float to remove the quotes from all type of numbers
  console.log("sendValue", command);
  
  requestJson(command);
}

let isModal = false;
let modalPlaceHolder;

function toggleModal(varNode) { //canvas or textarea
  // console.log("toggleModal", varNode);
  isModal = !isModal;

	if (isModal) {

    modalPlaceHolder = cE(varNode.nodeName.toLocaleLowerCase()); //create canvas or textarea
    modalPlaceHolder.width = varNode.width;
    modalPlaceHolder.height = varNode.height;

    varNode.parentNode.replaceChild(modalPlaceHolder, varNode); //replace by modalPlaceHolder

    // let btn = cE("button");
    // btn.innerText = "close";
    // btn.addEventListener('click', (event) => {toggleModal(varNode);});
    // gId('modalView').appendChild(btn);

    gId('modalView').appendChild(varNode);
    varNode.width = window.innerWidth;
    varNode.height = window.innerHeight;
    // console.log("toggleModal +", varNode, modalPlaceHolder, varNode.getBoundingClientRect(), modalPlaceHolder.getBoundingClientRect().width, modalPlaceHolder.getBoundingClientRect().height, modalPlaceHolder.width, modalPlaceHolder.height);
	}
  else {    
    varNode.width = modalPlaceHolder.getBoundingClientRect().width;
    varNode.height = modalPlaceHolder.getBoundingClientRect().height;

    // console.log("toggleModal -", varNode, modalPlaceHolder, varNode.getBoundingClientRect(), modalPlaceHolder.getBoundingClientRect().width, modalPlaceHolder.getBoundingClientRect().height, modalPlaceHolder.width, modalPlaceHolder.height);
    
    modalPlaceHolder.parentNode.replaceChild(varNode, modalPlaceHolder); // //replace by varNode. modalPlaceHolder loses rect
  }

	gId('modalView').style.transform = (isModal) ? "translateY(0px)":"translateY(100%)";
}
// https://stackoverflow.com/questions/324303/cut-and-paste-moving-nodes-in-the-dom-with-javascript

function initCap(s) {
  if (typeof s !== 'string') return '';
  // https://www.freecodecamp.org/news/how-to-capitalize-words-in-javascript/
  return s.replace(/[\W_]/g,' ').replace(/(^\w{1})|(\s+\w{1})/g, l=>l.toUpperCase()); // replace - and _ with space, capitalize every 1st letter
}


//drag and drop functionality
//===========================

var dragSrcEl;

// https://stackoverflow.com/questions/75698658/how-can-i-drag-and-drop-like-browser-tabs-in-javascript
function initMdlColumns() {

  let columns = gId("mdlContainer").childNodes;
  columns.forEach(function(column) {
    column.addEventListener('dragover', handleDragOver);
    column.addEventListener('dragenter', handleDragEnter);
    column.addEventListener('dragleave', handleDragLeave);
    column.addEventListener('drop', handleDrop);
  });

  setupModules();
  
}

function setupModules() {
  let columns = gId("mdlContainer").childNodes;
  columns.forEach(function(column) {
    let modules = column.childNodes;
    modules.forEach(function(module) {
      setupModule(module);
    });
  });
}

// var lastPage;
function setupModule(item) {
  item.addEventListener('dragstart', handleDragStart);
  item.addEventListener('dragover', handleDragOver);
  item.addEventListener('dragenter', handleDragEnter);
  item.addEventListener('dragleave', handleDragLeave);
  item.addEventListener('dragend', handleDragEnd);
  item.addEventListener('drop', handleDrop);
  // item.onclick = function() {
  //   console.log("click", this, lastPage);
  //   if (lastPage) document.getElementById(lastPage.id+"-page").hidden = true;
  //   document.getElementById(this.id+"-page").hidden = false;
  //   lastPage = this;
  // };
}

function handleDragStart(e) {
  this.style.opacity = '0.4';

  dragSrcEl = this;

  e.dataTransfer.effectAllowed = 'move';
  e.dataTransfer.setData('text/html', this.innerText);
  console.log("handleDragStart", this, e, e.dataTransfer);
  e.dataTransfer.setData('text/plain', this.id);
}

function removeDragStyle(item) {
  item.style.opacity = '1';

  let columns = gId("mdlContainer").childNodes;
  columns.forEach(function(column) {
    let modules = column.childNodes;
    modules.forEach(function(module) {
      if (module.classList) module.classList.remove('over'); //bug? dragLeave is called immediate so over has been removed already
    });
    if (column.classList) column.classList.remove('over');
  });
}

function handleDragEnd(e) {
  console.log("handleDragEnd", this, e);
  removeDragStyle(this);
}

function handleDragOver(e) {
  e.preventDefault();
  return false;
}

function handleDragEnter(e) {
  this.classList.add('over');
}

function handleDragLeave(e) {
  this.classList.remove('over');
}

function handleDrop(e) {
  e.stopPropagation();

  if (dragSrcEl !== this) {
    console.log("handleDrop", dragSrcEl, this, e, e.dataTransfer);

    const clone = dragSrcEl.cloneNode(true);
    setupModule(clone);
    removeDragStyle(clone);

    if (this.id.includes("mdlColumn")) {
      console.log("coladd");
      this.appendChild(clone);
    } else {
      this.parentNode.insertBefore(clone, this.nextSibling);
    }

    dragSrcEl.remove();
  }

  return false;
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
		callError("File " + name + " not found");
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

function setInstanceTableColumns() {

  let tbl = gId("insTbl");
  if (!tbl) return;
  let isDashView = gId("vDash").classList.contains("selected");
  let thead = tbl.querySelector("thead");
  let tbody = tbl.querySelector("tbody");

  function showHideColumn(colNr, doHide) {
    // console.log("showHideColumn", thead.parentNode.parentNode, colNr, doHide);
    if (colNr < thead.querySelector("tr").childNodes.length)
      thead.querySelector("tr").childNodes[colNr].hidden = doHide;
    for (let trNode of tbody.querySelectorAll("tr"))
      if (colNr < trNode.childNodes.length)
        trNode.childNodes[colNr].hidden = doHide;
  }

  // console.log("setInstanceTableColumns", tbl, thead, tbody);
  let columnNr = 3; //column 0, 1 and 2 will always be shown (name, show and link)
  for (; columnNr < 11; columnNr++) { // fixed columns which show in Systems/All and not in Dashboard tab
    showHideColumn(columnNr, isDashView);
  }
  for (; columnNr<thead.querySelector("tr").childNodes.length; columnNr++) {
    showHideColumn(columnNr, !isDashView);
  }
}

function changeHTMLView(viewName) {

  // console.log("changeHTMLView", node, node.value, node.id, mdlContainerNode, mdlContainerNode.childNodes);
  
  gId("vSetup").classList.remove("selected");
  gId("vApp").classList.remove("selected");
  gId("vDash").classList.remove("selected");
  gId("vUser").classList.remove("selected");
  gId("vSys").classList.remove("selected");
  gId("vAll").classList.remove("selected");
  gId(viewName).classList.add("selected");

  let mdlContainerNode = gId("mdlContainer"); //class mdlContainer

  let columnCounter = 0;
  for (let mdlColumnNode of mdlContainerNode.childNodes) {
    let mdlFound = false;
    for (let divNode of mdlColumnNode.childNodes) {
      let found = false;
      if (viewName == "vAll")
        found = true;
      else {
        for (let moduleNode of divNode.childNodes) {
          if (moduleNode.className) {
            if (viewName=="vSetup" && findVar(moduleNode.id).s) // show all module with setup variable (s) set to true
              found = true;
            if (viewName=="vApp" && moduleNode.className == "appmod")
              found = true;
            if (viewName=="vSys" && moduleNode.className == "sysmod")
              found = true;
            if (viewName=="vUser" && moduleNode.className == "usermod")
              found = true;
            if (viewName=="vDash" && moduleNode.id == "Instances")
              found = true;
          }
          // console.log(mdlColumnNode, moduleNode, moduleNode.className);
        }
      }
      divNode.hidden = !found;
      if (found) mdlFound = true;
    }

    mdlColumnNode.hidden = !mdlFound;
    if (mdlFound) {
      columnCounter++;
    }
  }

  // if (viewName=="vApp")
  //   mdlContainerNode.className = "mdlContainer2";
  // else
    mdlContainerNode.className = "mdlContainer" + columnCounter; //1..4

  setInstanceTableColumns();

} //changeHTMLView

//https://webdesign.tutsplus.com/color-schemes-with-css-variables-and-javascript--cms-36989t
function changeHTMLTheme(themeName) {
  localStorage.setItem('theme', themeName);
  document.documentElement.className = themeName;
  if (gId("theme-select").value != themeName)
    gId("theme-select").value = themeName;
}

function saveModel(node) {
  console.log("saveModel", node);

  sendValue(node);
}

function setView(node) {
  var command = {};
  command.view = node.id;
  requestJson(command);
}

function setTheme(node) {
  var command = {};
  command.theme = node.value;
  requestJson(command);
}

function getTheme() {
  let value = localStorage.getItem('theme');
  if (value && value != "null") changeHTMLTheme(value);
}

function previewBoard(canvasNode, buffer) {
  let boardColor;
  if (sysInfo.board == "esp32s2") boardColor = "purple";
  else if (sysInfo.board == "esp32s3") boardColor = "blue";
  else boardColor = "green";

  let ctx = canvasNode.getContext('2d');
  //assuming 20 pins
  let mW = sysInfo.nrOfPins<=40?2:4; // nr of pin columns
  let mH = sysInfo.nrOfPins / mW; // pins per column
  let pPL = Math.min(canvasNode.width / mW, canvasNode.height / (mH+2)); // pixels per LED (width of circle)
  let bW = pPL*10;
  let bH = mH * pPL;
  let lOf = Math.floor((canvasNode.width - bW) / 2); //left offset (to center matrix)
  // let i = 5;

  let pos = {};

  ctx.clearRect(0, 0, canvasNode.width, canvasNode.height);

  pos.x = lOf; pos.y = pPL;
  //board
  ctx.beginPath();
  ctx.fillStyle = boardColor;
  ctx.fillRect(pos.x, pos.y, bW, bH);

  //wifi
  ctx.fillStyle = "darkBlue";
  if (mW == 2)
    ctx.fillRect(pos.x + 1.5*pPL, 0, pPL * 7, pPL * 3);
  else
    ctx.fillRect(pos.x + 2.5*pPL, 0, pPL * 5, pPL * 3);

  //cpu
  ctx.fillStyle = "gray";
  if (mW == 2)
    ctx.fillRect(pos.x + 1.5*pPL, pos.y + 3*pPL, pPL * 7, pPL * 7);
  else
    ctx.fillRect(pos.x + 2.5*pPL, pos.y + 3*pPL, pPL * 5, pPL * 5);


  //esp32 text
  ctx.beginPath();
  ctx.font = pPL *1.5 + "px serif";
  ctx.fillStyle = "black";
  ctx.textAlign = "center";
  ctx.fillText(sysInfo.board, pos.x + 5*pPL, pos.y + 6 * pPL);

  //chip
  if (mW == 2) { //no space if 4 columns
    ctx.fillStyle = "black";
    ctx.fillRect(pos.x + 6 * pPL, pos.y + 12*pPL, pPL * 2, pPL * 5);
  }
  
  //usb
  ctx.fillStyle = "grey";
  ctx.fillRect(pos.x + 3.5 * pPL, bH - pPL, pPL * 3, pPL * 3);
  
  let index = 0;
  for (let x = 0; x < mW; x++) {
    for (let y = 0; y < mH; y++) {
      let pinType = sysInfo.pinTypes[index];
      let pinColor = [];
      switch (pinType) {
        case pinTypeIO:
          pinColor = [78,173,50]; // green
          break;
        case pinTypeReadOnly:
          pinColor = [218,139,49];//"orange";
          break;
        case pinTypeReserved:
          pinColor = [156,50,246];//"purple";
          break;
        default:
          pinColor = [192,48,38];//"red";
      }
      ctx.fillStyle = `rgba(${pinColor[0]},${pinColor[1]},${pinColor[2]},${buffer[index + 5]})`;
      pos.y = (y+0.5)*pPL + pPL;
      ctx.beginPath();
      if ((x == 0 && mW == 2) || (x <= 1 && mW == 4))
        pos.x = (x+0.5)*pPL + lOf;
      else if (mW == 2)
        pos.x = (x+0.5)*pPL + lOf + 8 * pPL;
      else
        pos.x = (x+0.5)*pPL + lOf + 6 * pPL;
      ctx.arc(pos.x, pos.y, pPL * 0.4, 0, 2 * Math.PI);
      ctx.fill();

      ctx.beginPath();
      ctx.font = pPL*0.5 + "px serif";
      ctx.fillStyle = "black";
      ctx.textAlign = "center";
      ctx.fillText(index, pos.x, pos.y + pPL / 4);

      index++;
    }
  }
}

// Utility function
//https://stackoverflow.com/questions/8511281/check-if-a-value-is-an-object-in-javascript
function isObject(val) {
  if (Array.isArray(val)) return false;
  if (val === null) { return false;}
  return ( (typeof val === 'function') || (typeof val === 'object'));

  //or   return obj === Object(obj); //true for arrays.???
}