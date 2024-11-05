// @title     StarBase
// @file      modules.js
// @date      20241105
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

const pinTypeIO = 0;
const pinTypeReadOnly = 1;
const pinTypeReserved = 2;
const pinTypeSpi = 3;
const pinTypeInvalid = UINT8_MAX;

class Modules {

  model = [] //model.json (as send by the server), used by FindVar

  //generates html for a module
  //for some strange reason, this method does not accept this.otherMethodInThisClass ???
  createHTML(moduleJson) {
    let code = ""
    controller.modules.test("test")
    // this.test("ew") //this.otherMethodInThisClass: [Error] Unhandled Promise Rejection: TypeError: this.test is not a function. (In 'this.test("ew")', 'this.test' is undefined)

    // sort module variables
    moduleJson.n.sort(function(a,b) {
      return Math.abs(a.o) - Math.abs(b.o); //o is order nr (ignore negatives for the time being)
    });

    for (let variable of moduleJson.n) {
      let variableClass = varJsonToClass(variable);
      code += variableClass.createHTML()
    }
    code += '<pre>' + JSON.stringify(moduleJson, null, 2) + '</pre>'
    return code
  }

  //done after innerHTML as it needs to find the nodes. tbd: createHTMLFun adds to dom directly
  setDefaultValues(moduleJson) {
    // console.log("setDefaultValues", moduleJson)
    controller.modules.walkThroughModel(function(variable) { //this.walkThroughModel not working for some reason???
      
      if (Array.isArray(variable.value)) variable.value = variable.value[0] //temp hack to deal with table values

      if (variable.value != null) {
        let variableClass = varJsonToClass(variable);
        variableClass.receiveData({"value":variable.value}); //receiveData knows how the value should be assigned to the node
      }
    }, moduleJson.n)
  }

  //temporary to test issue above
  test(name) {
    // console.log(name)
  }

  //used by fetchModel and by makeWS
  addModule(moduleJson) { 
    moduleJson.type = moduleJson.type=="appmod"?appName():moduleJson.type=="usermod"?"User":"System"; 
    this.model.push(moduleJson);

    //updateUI is made after all modules have been fetched, how to adapt to add one module?
    controller.mainNav.updateUI(moduleJson, this.createHTML, this.setDefaultValues); //moduleFun returns the html to show in the module panel of the UI
    //still doesn't maker sense  to call updateUI for every module ...
  }

  //walkthrough, if a variable is returned by fun, it stops the walkthrough with this variable
  walkThroughModel(fun, parent = this.model) {
    for (let variable of parent) {
      let foundVar = null
      foundVar = fun(variable);

      if (foundVar) return foundVar;
 
      if (variable.n) {
        foundVar = this.walkThroughModel(fun, variable.n); //recursive
        if (foundVar) return foundVar;
      }
    }
  }

  //loop through the model hierarchy and generateData data for each variable in the model
  generateData() {
    this.walkThroughModel(function(variable) {
      let variableClass = varJsonToClass(variable);
      variableClass.generateData();
    })
  }

  //finds a var with id in the model, if found returns it
  findVar(pid, id) {
    // console.log("findVar", id, parent, model);
    return this.walkThroughModel(function(variable){
      if (variable.pid == pid, variable.id == id) //found variable
        return variable; //this stops the walkThrough
    })
  }

  previewBoard(canvasNode, buffer) {
    let boardColor;
    if (controller.sysInfo.board == "esp32s2") boardColor = "purple";
    else if (controller.sysInfo.board == "esp32s3") boardColor = "blue";
    else boardColor = "green";
    // console.log("previewBoard", buffer, controller.sysInfo)
  
    let ctx = canvasNode.getContext('2d');
    //assuming 20 pins
    let mW = controller.sysInfo.nrOfPins<=40?2:4; // nr of pin columns
    let mH = controller.sysInfo.nrOfPins / mW; // pins per column
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
    ctx.fillText(controller.sysInfo.board, pos.x + 5*pPL, pos.y + 6 * pPL);
  
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
        let pinType = controller.sysInfo.pinTypes[index];
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
  } //previewBoard
  
} //class Modules

//see this.otherMethodInThisClass, should be moved in Modules class
//creates the right class based on variable.type
function varJsonToClass(variable) {
  switch (variable.type) {
    case "text":
      return new TextVariable(variable);
    case "checkbox":
      return new CheckboxVariable(variable);
    case "range":
      return new RangeVariable(variable);
    case "button":
      return new ButtonVariable(variable);
    case "select":
      return new SelectVariable(variable);
    case "progress":
      return new ProgressVariable(variable);
    case "canvas":
      return new CanvasVariable(variable);
    case "coord3D":
      return new Coord3DVariable(variable);
    case "table":
      return new TableVariable(variable);
    default:
      return new Variable(variable);
  }
}

class Variable {

  constructor(variable) {
    this.variable = variable
    this.node = gId(this.variable.pid + "." + this.variable.id);
  }

  createHTML(node = `<input id="${this.variable.pid + "." + this.variable.id}" type="${this.variable.type}" class="${this.variable.type}" value="${this.variable.value}"></input>`) {
    let code = `<p>
            <label>${initCap(this.variable.id)}</label>
            ${node}
            <p>`

    //ask server for onUI
    let command = {};
    command.onUI = [this.variable.pid + "." + this.variable.id];
    controller.requestJson(command); // this can be done here because of the async nature of requestJson, better is to do it after innerHTML+=...

    if (this.variable.n) {
      code += `<div id="${this.variable.pid + "." + this.variable.id}_n" class="ndiv">`
      for (let childVar of this.variable.n) {
        let childClass = varJsonToClass(childVar)
        code += childClass.createHTML();
      }
      code += `</div>`
    }
    return code
}

  //sets the value of the node to value of properties
  receiveData(properties) {
    if (this.node) {
      if (properties.label) {
        let labelNode = this.node.parentNode.querySelector("label")
        if (labelNode)
          labelNode.innerText = properties.label;
        else
          this.node.parentNode.innerHTML = `<label>${properties.label}</label>` + this.node.parentNode.innerHTML
      }

      if (Array.isArray(properties.value)) properties.value = properties.value[0] //temp hack to deal with table values

      if (properties.value != null && this.node.value != null) {
        this.node.value = properties.value;
      }
      if (properties.comment) {
        let commentNode = this.node.parentNode.querySelector("comment")
        if (!commentNode) {
          commentNode = cE("comment")
          this.node.parentNode.appendChild(commentNode) // disturbing canvas: this.node.parentNode.innerHTML += `<comment>${properties.comment}</comment>`
        }        
        commentNode.innerText = properties.comment
      }
      if (properties.file) {
        let url;
        if (window.location.href.includes("127.0.0.1"))
          url = "/misc/" //get the files from the misc folder in the repo
        else 
          url = `http://${window.location.hostname}/file/`

        fetchAndExecute(url, properties.file, this.variable, function(variable, text) {
          variable.file = JSON.parse(text); //assuming it is json, should we call this file?
          variable.file.new = true;
          console.log("receiveData file fetched", variable.id, variable.file);
        }); 
      }
    } 
  }

  //used by Live Server
  generateData(custom = `"value":${Math.random() * 256}`) {
    if (custom != "") custom += ", "
    controller.receiveData(JSON.parse(`{"${this.variable.pid + "." + this.variable.id}":{${custom}"comment":"${Math.random().toString(36).slice(2)}"}}`));
  }

} //class Variable

class TextVariable extends Variable {

  generateData() {
    super.generateData(`"value":"${Math.random().toString(36).slice(2)}"`)
  }

} //class TextVariable

class CheckboxVariable extends Variable {

  createHTML() {
    return super.createHTML(`<input id="${this.variable.pid + "." + this.variable.id}" type="${this.variable.type}" class="${this.variable.type}"></input><span class="checkmark"></span>`);
  }

  receiveData(properties) {
    super.receiveData(properties)
    if (this.node && properties.value != null) { //
      this.node.checked = properties.value
      this.node.indeterminate = (properties.value == -1); //tbd: gen -1
    }
  }

  generateData() {
    super.generateData(`"value":${Math.random() <.33?-1:Math.random() <.66?0:1}`)
  }

} //class CheckboxVariable

class RangeVariable extends Variable {

  createHTML() {
    return super.createHTML(`<input id="${this.variable.pid + "." + this.variable.id}" type="${this.variable.type}" min="0" max="255" class="${this.variable.type}"></input><span id="${this.variable.pid + "." + this.variable.id}_rv">${this.variable.value}</span>`)
  }

  receiveData(properties) {
    super.receiveData(properties)
    let rvNode = gId(this.variable.pid + "." + this.variable.id + "_rv")
    if (rvNode && properties.value != null)
      rvNode.innerText = properties.value
  }

  generateData() {
    super.generateData(`"value":${Math.round(Math.random()*255)}`) //no value update
  }

} //class RangeVariable

class ButtonVariable extends Variable {

  createHTML() {
    return super.createHTML(`<input id="${this.variable.pid + "." + this.variable.id}" type="${this.variable.type}" class="${this.variable.type}" value="${initCap(this.variable.id)}"></input>`)
  }

  generateData() {
    super.generateData("") //no value update
  }

} //class ButtonVariable

class SelectVariable extends Variable {

  createHTML() {
    return super.createHTML(`<select id="${this.variable.pid + "." + this.variable.id}" class="select"></select>`)
  }

  receiveData(properties) {
    super.receiveData(properties)
    // if (this.node && properties.value != null)
    //   console.log("select.receiveData value", this.node, properties)
    if (this.node && properties.options != null) {
      this.node.innerHTML = ""
      let optionCounter = 0;
      for (let option of properties.options)
        this.node.innerHTML+=`<option value=${optionCounter++}>${option}</option>`
      this.node.value = this.variable.value //select the right option
    }
  }

  generateData() {
    //add sample options if not yet existing
    if (this.node && this.node.childNodes.length == 0)
      super.generateData(`"options":["ein","zwei","drei"]`)

    super.generateData(`"value":${Math.random() <.33?0:Math.random() <.66?1:2}`)
  }

} //class SelectVariable

class ProgressVariable extends Variable {

  createHTML() {
    return super.createHTML(`<progress max="${this.variable.max}" id="${this.variable.pid + "." + this.variable.id}" class="progress"></progress>`)
  }

} //class ProgressVariable

class CanvasVariable extends Variable {

  createHTML() {
    // console.log(this.variable)
    if (this.variable.file) {
      this.variable.file.new = true;
      // console.log("canvas createHTML", this.variable.file, this.variable.file.new);
    }
    return super.createHTML(`<canvas id="${this.variable.pid + "." + this.variable.id}" class="${this.variable.type}"></canvas>`);
  }

  receiveData(properties) {
    super.receiveData(properties)
  }

  generateData() {
    //LEDs specific
    if (this.node && this.variable.file == null)
      super.generateData(`"file":"F_panel2x2-16x16.json"`)
    //end LEDs specific

    super.generateData(`"value":"n/a"`) //no value needed for canvas, but only comment update ...
  }

} //class CanvasVariable

class TableVariable extends Variable {

  createHTML() {
    return super.createHTML(`<table id="${this.variable.pid + "." + this.variable.id}" class="${this.variable.type}"></table>`);
  }

  generateData() {
    super.generateData(`"value":"n/a"`) //no value needed for canvas...
  }

} //class TableVariable

class Coord3DVariable extends Variable {

  createHTML() {
    return super.createHTML(`<span id="${this.variable.pid + "." + this.variable.id}" class="${this.variable.type}"><input type="number" placeholder="x"/><input type="number" placeholder="y"/><input type="number" placeholder="z"/></span>`);
  }

  receiveData(properties) {
    super.receiveData(properties)
    if (this.node && properties.value != null) {
      this.node.childNodes[0].value = properties.value.x
      this.node.childNodes[1].value = properties.value.y
      this.node.childNodes[2].value = properties.value.z
    }
  }

  generateData() {
    super.generateData(`"value":{"x":${Math.random()*360}, "y":${Math.random()*360}, "z":${Math.random()*360}}`)
  }

} //class Coord3DVariable

