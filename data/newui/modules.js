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
  createHTML(moduleJson, parentNode) {
    parentNode.innerHTML = `<div id="m.${moduleJson.id}"></div>`
    controller.modules.test("test")
    // this.test("ew") //this.otherMethodInThisClass: [Error] Unhandled Promise Rejection: TypeError: this.test is not a function. (In 'this.test("ew")', 'this.test' is undefined)

    // sort module variables
    moduleJson.n.sort(function(a,b) {
      return Math.abs(a.o) - Math.abs(b.o); //o is order nr (ignore negatives for the time being)
    });

    //all variables of module
    for (let variable of moduleJson.n) {
      let variableClass = varJsonToClass(variable);
      variableClass.createHTML(gId(`m.${moduleJson.id}`))
    }

    gId(`m.${moduleJson.id}`).innerHTML += '<pre>' + JSON.stringify(moduleJson, null, 2) + '</pre>'
  }

  //done after innerHTML as it needs to find the nodes. tbd: createHTMLFun adds to dom directly
  setDefaultValues(moduleJson) {
    return;
    // console.log("setDefaultValues", moduleJson)
    controller.modules.walkThroughModel(function(parent, variable) { //this.walkThroughModel not working for some reason???
      
      // if (Array.isArray(variable.value)) variable.value = variable.value[0] //temp hack to deal with table values

      if (variable.value != null) {
        let variableClass = varJsonToClass(variable);
        variableClass.receiveData({"value":variable.value}); //receiveData knows how the value should be assigned to the node
      }
    }, moduleJson)
  }

  //temporary to test issue above
  test(name) {
    // console.log(name)
  }

  //used by fetchModel and by makeWS
  addModule(moduleJson) { 
    moduleJson.type = moduleJson.type=="appmod"?appName():moduleJson.type=="usermod"?"User":"System"; //to display proper name
    this.model.push(moduleJson);

    //updateUI is made after all modules have been fetched, how to adapt to add one module?
    controller.mainNav.updateUI(moduleJson, this.createHTML, this.setDefaultValues); //moduleFun returns the html to show in the module panel of the UI
    //still doesn't maker sense  to call updateUI for every module ...
  }

  //walkthrough, if a variable is returned by fun, it stops the walkthrough with this variable
  walkThroughModel(fun, parent = null) {
    for (let variable of parent?parent.n:this.model) {

      let foundVar = fun(parent, variable);
      if (foundVar) return foundVar;
 
      if (variable.n) {
        foundVar = this.walkThroughModel(fun, variable); //recursive
        if (foundVar) return foundVar;
      }
    }
  }

  //loop through the model hierarchy and generateData data for each variable in the model
  generateData() {
    this.walkThroughModel(function(parent, variable) {
      if (parent) { //no need to send modules
        let variableClass = varJsonToClass(variable);
        // console.log(variableClass.pidid())
        variableClass.generateData();
      }
    })
  }

  //finds a var with id in the model, if found returns it
  findVar(pid, id) {
    // console.log("findVar", id, parent, model);
    return this.walkThroughModel(function(parent, variable) {
      if (variable.pid == pid && variable.id == id) //found variable
        return variable; //this stops the walkThrough
    })
  }
  findParentVar(pid, id) {
    // console.log("findVar", id, parent, model);
    return this.walkThroughModel(function(parent, variable) {
      if (parent && parent.id == pid && variable.id == id) //found variable
        return parent; //this stops the walkThrough
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
    case "number":
      return new NumberVariable(variable);
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
    this.node = gId(this.pidid()); //for tables this is the header row
  }

  pidid(rowNr = UINT8_MAX) {
    if (rowNr == UINT8_MAX)
      return this.variable.pid + "." + this.variable.id
    else
      return this.variable.pid + "." + this.variable.id + "#" +  rowNr
  }

  createNode(parentNode, rowNr = UINT8_MAX) {
    let node
    if (this.variable.ro) {
      node = `<span id="${this.pidid(rowNr)}" class="${this.variable.type}">${this.variable.value}</span>`
    } else {
      node = `<input id="${this.pidid(rowNr)}" type="${this.variable.type}" class="${this.variable.type}" value="${this.variable.value}"></input>`
    }
    parentNode.innerHTML += node
  }

  createHTML(parentNode, rowNr = UINT8_MAX) {

    // console.log("createHTML2", this.pidid(rowNr))

    parentNode.innerHTML += `<div id = "${this.pidid(rowNr)}_d"></div>`

    let divNode = gId(`${this.pidid(rowNr)}_d`)

    if (parentNode.nodeName != "TD") //replace by parentVar...
      divNode.innerHTML += `<label>${initCap(this.variable.id)}</label>`

    this.createNode(divNode, rowNr)

    // if (!this.node) {
    //   this.node = gId(this.pidid(rowNr))
    //   console.log("new node", this.node, this.variable)
    // }

    //set the default value for this class
    if (this.variable.value) {
      console.log("init value", this.pidid(rowNr), this.variable.value)
      if (rowNr == UINT8_MAX)
        this.receiveValue(this.variable.value)
      else 
        this.receiveValue(this.variable.value[rowNr], rowNr)
    }

    if (this.variable.n && this.variable.type != "table") { // a table will create it's own columns in createNode2

      divNode.innerHTML += `<div id="${this.pidid(rowNr)}_n" class="ndiv"></div>`
  
      for (let childVar of this.variable.n) {
        let childClass = varJsonToClass(childVar)
        childClass.createHTML(gId(this.pidid(rowNr) + "_n"), rowNr);
      }
    }

    //ask server for onUI, not if parent is table...
    if (rowNr == UINT8_MAX) {
      let command = {};
      command.onUI = [this.pidid()];
      controller.requestJson(command); // this can be done here because of the async nature of requestJson, better is to do it after innerHTML+=...
      //this triggers receiveData for this class which processes onUI result (options, comment etc)
      }
  
      
  }

  receiveValue(value, rowNr = UINT8_MAX) {
    // console.log("Variable receiveValue", childVar, values)
    // console.log(childVar.pidid())

    let node = gId(this.pidid(rowNr))

    if (node) {
      if (node.nodeName == "SPAN") {
        if (node.innerHTML != null)
          node.innerHTML = value;
      } else  {
        if (node.value != null)
          node.value = value;
      }
    }
  }

  //add comment, if not exist, create comment
  receiveComment(comment) {
    // return;
    if (comment) {
      //if not tablecolumn
      // let parentVar = controller.modules.findParentVar(this.variable.pid, this.variable.id);

      // if (parentVar.type != "table") {
        let commentNode = this.node.parentNode.querySelector("comment")
        if (!commentNode) {
          commentNode = cCE(this.node.parentNode, "comment") // disturbing canvas: this.node.parentNode.innerHTML += `<comment>${properties.comment}</comment>`
        }        
        commentNode.innerText = comment
      // }
      // else
        // to do add to th
    }
  }

  receiveData(properties) {

    if (this.node) {

      let parentClass = varJsonToClass(controller.modules.findParentVar(this.variable.pid, this.variable.id))

      if (properties.label) {
        let labelNode = this.node.parentNode.querySelector("label")
        if (labelNode)
          labelNode.innerText = properties.label;
        else
          this.node.parentNode.innerHTML = `<label>${properties.label}</label>` + this.node.parentNode.innerHTML
      }

      if (properties.value != null) { //!= 0 to include 0

        //parent is table (multirow)
        if (Array.isArray(properties.value)) {
          // console.log("receiveValue2 array", this.pidid(), properties.value)
          // console.log("receiveData2", this.pidid(), properties.value)

          //make sure node exists ???
          parentClass.receiveValue(properties.value) //table: create extra rows if needed

          //receive value for each cell
          for (let rowNr = 0; rowNr < properties.value.length; rowNr++) {
            this.receiveValue(properties.value[rowNr], rowNr)
          }
        }
        else {
          // console.log("receive value for", this.pidid(), properties.value)
          this.receiveValue(properties.value)
        }
      }

      if (properties.comment) {
        console.log("receive comment for", this.pidid(), properties.comment)
        if (parentClass.variable.type != "table")
          this.receiveComment(properties.comment)
        else {
          //place comment under th columns ...
        }
      }

      if (properties.file)
        this.receiveFile(properties.file)
    } //if node
  }

  generateValue() {
    return `${Math.round(Math.random()*255)}`
  }

  //generateData: sends data to controller.receiveData which dispatches it to the right variable (used by Live Server)
  //valueFun: function to generate data for one cell
  //extra: e.g. in case of SelectVariable: the options of a drop down
  //if table variable then an array of these values will be send (table column, receiveData will put them in cells)
  generateData(extra = "") {
    let value = ""
    let sep = ""
    if (this.generateValue()) {
      let parentVar = controller.modules.findParentVar(this.variable.pid, this.variable.id);
      if (parentVar.type != "table")
        value = `${sep} "value":${this.generateValue()}`
      else
        value = `${sep} "value":[${this.generateValue()}, ${this.generateValue()}, ${this.generateValue()}]`
      sep = `,`
    }

    if (extra) {
      extra = `${sep}` + extra
      sep = ","
    }

    let comment = `${sep} "comment": "${this.variable.id + " " + Math.random().toString(36).slice(2)}"`

    // console.log(`{"${this.pidid()}":{${value} ${extra} ${comment}}}`)

    //check if variable part of table
    controller.receiveData(JSON.parse(`{"${this.pidid()}":{${value} ${extra} ${comment}}}`));
  
  }

} //class Variable

class TextVariable extends Variable {

  generateValue() {
    return `"${Math.random().toString(36).slice(2)}"`
  }

} //class TextVariable

class NumberVariable extends Variable {

  createNode(parentNode, rowNr = UINT8_MAX) {
    let node
    if (this.variable.ro) {
      node = `<span id="${this.pidid(rowNr)}" class="${this.variable.type}">${this.variable.value}</span>`
    } else {
      node = `<input id="${this.pidid(rowNr)}" type="${this.variable.type}" class="${this.variable.type}" value="${this.variable.value}" min="${this.variable.min}" max="${this.variable.max}""></input>`
    }
    // xNode.min = variable.min?variable.min:0; //if not specified then unsigned value (min=0)
    // if (variable.max) xNode.max = variable.max;
    parentNode.innerHTML += node
  }

  generateValue() {
    return `${Math.round(Math.random()*255)}`
  }

} //class TextVariable

class CheckboxVariable extends Variable {

  createNode(parentNode, rowNr = UINT8_MAX) {
    parentNode.innerHTML += `<input id="${this.pidid(rowNr)}" + type="${this.variable.type}" class="${this.variable.type}"></input><span class="checkmark"></span>`

  }

  receiveValue(value, rowNr = UINT8_MAX) {
    let node = gId(this.pidid(rowNr))
    
    if (node && value != null) {
      console.log("rv2", this.pidid(rowNr), value)
      // console.log("CheckboxVariable receiveValue", this.pidid(rowNr), value)
      node.checked = value
      node.indeterminate = (value == -1); //tbd: gen -1
      if (this.variable.ro)
        node.disabled = true
    }
  }

  generateValue() {
    return `${Math.random() <.33?-1:Math.random() <.66?0:1}` // -1, 0 and 1
  }

} //class CheckboxVariable

class RangeVariable extends Variable { //slider

  createNode(parentNode, rowNr = UINT8_MAX) {
    parentNode.innerHTML += `<input id="${this.pidid(rowNr)}" type="${this.variable.type}" min="0" max="255" class="${this.variable.type}"></input><span id="${this.pidid(rowNr)}_rv">${this.variable.value}</span>`
  }

  receiveValue(value, rowNr = UINT8_MAX) {
    super.receiveValue(value, rowNr)

    let rvNode = gId(this.pidid(rowNr) + "_rv")
    // console.log(rvNode, value)
    if (rvNode && value != null)
      rvNode.innerText = value
  }

  generateValue() {
    return `${Math.round(Math.random()*255)}`
  }

} //class RangeVariable

class ButtonVariable extends Variable {

  createNode(parentNode, rowNr = UINT8_MAX) {
    parentNode.innerHTML += `<input id="${this.pidid(rowNr)}" type="${this.variable.type}" class="${this.variable.type}" value="${initCap(this.variable.id)}"></input>`
  }

  generateValue() {
    return null //no values
  }

} //class ButtonVariable

class SelectVariable extends Variable {

  createNode(parentNode, rowNr = UINT8_MAX) {
    if (this.variable.ro)
      parentNode.innerHTML += `<span id="${this.pidid(rowNr)}" class="${this.variable.type}">${this.variable.value}</span>`
    else
    parentNode.innerHTML += `<select id="${this.pidid(rowNr)}" class="${this.variable.type}" value="${this.variable.value}"></select>`
  }

  receiveValue(value, rowNr = UINT8_MAX) {
    // console.log("selectVar receiveValue", this.node, value, rowNr)

    let node = gId(this.pidid(rowNr))
    if (node) {
      let options = node.getAttribute("options")
      if (options) {
        options = options.split(",")
        if (this.variable.ro) {
          if (options) {
            let optionCounter = 0;
            for (let option of options) {
              // console.log(option, properties.value)
              if (optionCounter == value)
                node.innerText = option
              optionCounter++;
            }
          }
        } else {
          if (!node.innerHTML) {
            node.innerHTML = ""
            let optionCounter = 0;
            for (let option of options)
              node.innerHTML+=`<option value=${optionCounter++}>${option}</option>`
          }
          if (value)
            node.value = value //select the right option
        }
      }
    }
  }

  receiveData(properties) {
    // if (this.variable.id == "effect")
    //   console.log(this.pidid(), properties)

    if (this.node && properties.options != null) {
      if (Array.isArray(properties.value)) {
        for (let rowNr = 0; rowNr < properties.value.length; rowNr++) {
          let node = gId(this.pidid(rowNr))
          // console.log(node, this.pidid(rowNr))
          if (node) node.setAttribute("options", properties.options) //not always created yet. todo find out why
        }
      }
      else {
        this.node.setAttribute("options", properties.options)
        // console.log(properties.options, this.node.getAttribute("options"));
      }
    }
    super.receiveData(properties)
  }

  generateValue() {
    return `${Math.random() <.33?0:Math.random() <.66?1:2}` //0, 1, or 2
  }

  generateData() {
    //add sample options
    super.generateData(`"options":["eins","zwei","drei"]`)
  }

} //class SelectVariable

class ProgressVariable extends Variable {

  createNode(parentNode, rowNr = UINT8_MAX) {
    parentNode.innerHTML += `<progress max="${this.variable.max}" id="${this.pidid(rowNr)}" class="progress"></progress>`
  }

} //class ProgressVariable

class CanvasVariable extends Variable {

  createNode(parentNode, rowNr = UINT8_MAX) {
    parentNode.innerHTML += `<canvas id="${this.pidid(rowNr)}" class="${this.variable.type}"></canvas>`
  }

  createHTML(parentNode, rowNr = UINT8_MAX) {
    // console.log(this.variable)
    if (this.variable.file) {
      this.variable.file.new = true;
      // console.log("canvas createHTML", this.variable.file, this.variable.file.new);
    }
    super.createHTML(parentNode, rowNr);
  }

  generateValue() {
    return null;//`"n/a"`
  }

} //class CanvasVariable

class TableVariable extends Variable {

  createNode(parentNode, rowNr = UINT8_MAX) {

    let tableNode = cCE(parentNode, "table");
    tableNode.id = this.pidid()
    tableNode.className = this.variable.type
    cCE(tableNode, "tbody")
    // code += `<table id="${this.pidid()}" class="${this.variable.type}">`

    if (this.variable.n) {
      let trNode = cCE(cCE(tableNode, "thead"), "tr")
    
      for (let childVar of this.variable.n) {

        let thNode = cCE(trNode, "th")
        thNode.id = childVar.pid + "." + childVar.id
        thNode.className = childVar.type
        thNode.innerHTML = initCap(childVar.id)
        // code += `<th id="${childVar.pid + "." + childVar.id}" class="${childVar.type}">` // width="${childVar.max?childVar.max*3:100}"
        // code += initCap(childVar.id)
        // code += `</th>`
        
        let childClass = varJsonToClass(childVar)
        if (childVar.value) {
          console.log("childclass init value", childClass.pidid(rowNr), childVar.value)
          if (rowNr != UINT8_MAX)
            this.receiveValue(this.variable.value[rowNr], rowNr)
          else 
            childClass.receiveData({"value": childVar.value}) // will also create the table cells (which cause recursiveness...)
        }
    
      }
    }

  }

  //receive value array so enough rows can be created before the real values come in
  receiveValue(values, rowNr = UINT8_MAX) { //no rowNr used here
    //create missing rows if there are
    if (Array.isArray(values)) {
      let tbodyNode = this.node.querySelector("tbody");
      let trNodes = tbodyNode.querySelectorAll("tr");
      
      for (let rowNr = trNodes.length; rowNr < values.length; rowNr++) {
        // console.log("receiveValue2 table", this.pidid(), values, rowNr, values.length)
        let trNode = cCE(tbodyNode, "tr")
        for (let childVar of this.variable.n) {
          let tdNode = cCE(trNode, "td")
          let childVariable = varJsonToClass(childVar)
          // console.log("create cell", childVariable.pidid(rowNr))
          childVariable.createHTML(tdNode, rowNr, true) // this will call this method recursively... and also create missing rows
          //create a cell and it's children, will again this function via table.rv2
          //new cell will receive values
          //also new setValue will be called

        }
      }

      //todo: delete rows not needed

    } //isArray
  }

  generateData() {
    //only table columns have data, so do nothing
  }

} //class TableVariable

    // {/* <div id="Files.files_d">
    //   <label>Files</label>
    //   <table id="Files.files" class="table">
    //       <thead>
    //         <tr>
    //             <th id="files.name" class="text">Name</th>
    //             <th id="files.size" class="number">Size</th>
    //             <th id="files.time" class="number">Time</th>
    //             <th id="files.edit" class="fileEdit">Edit</th>
    //             <th>-</th>
    //         </tr>
    //       </thead>
    //       <tbody>
    //         <tr>
    //             <td>
    //               <div id="files.name#0_d"><span id="files.name#0" class="text">F_Panel2x2-16x16.json</span></div>
    //             </td>
    //             <td>
    //               <div id="files.size#0_d"><span id="files.size#0" class="number">9748</span></div>
    //             </td>
    //             <td>
    //               <div id="files.time#0_d"><span id="files.time#0" class="number">78</span></div>
    //             </td>
    //             <td>
    //               <div id="files.edit#0_d"><input type="button" value="🔍" id="files.edit#0" class="fileEdit" fname="F_Panel2x2-16x16.json"></div>
    //             </td>
    //             <td><input id="Files.files#0_del" type="button" value="-"></td>
    //         </tr>
    //         <tr>
    //             <td>
    //               <div id="files.name#1_d"><span id="files.name#1" class="text">F_panel080-048.sc</span></div>
    //             </td>
    //             <td>
    //               <div id="files.size#1_d"><span id="files.size#1" class="number">672</span></div>
    //             </td>
    //             <td>
    //               <div id="files.time#1_d"><span id="files.time#1" class="number">130</span></div>
    //             </td>
    //             <td>
    //               <div id="files.edit#1_d"><input type="button" value="🔍" id="files.edit#1" class="fileEdit" fname="F_panel080-048.sc"></div>
    //             </td>
    //             <td><input id="Files.files#1_del" type="button" value="-"></td>
    //         </tr>
    //         <tr>
    //             <td>
    //               <div id="files.name#2_d"><span id="files.name#2" class="text">model.json</span></div>
    //             </td>
    //             <td>
    //               <div id="files.size#2_d"><span id="files.size#2" class="number">11681</span></div>
    //             </td>
    //             <td>
    //               <div id="files.time#2_d"><span id="files.time#2" class="number">23</span></div>
    //             </td>
    //             <td>
    //               <div id="files.edit#2_d"><input type="button" value="🔍" id="files.edit#2" class="fileEdit" fname="model.json"></div>
    //             </td>
    //             <td><input id="Files.files#2_del" type="button" value="-"></td>
    //         </tr>
    //       </tbody>
    //   </table>
    //   <comment>List of files</comment>
    // </div> */}
    
class Coord3DVariable extends Variable {

  createNode(parentNode, rowNr = UINT8_MAX) {
    parentNode.innerHTML += `<span id="${this.pidid(rowNr)}" class="${this.variable.type}"><input type="number" min="0" max="65536" placeholder="x"/><input type="number" min="0" max="65536" placeholder="y"/><input type="number" min="0" max="65536" placeholder="z"/></span>`
  }

  receiveValue(value, rowNr = UINT8_MAX) {
    // console.log(childVar.pidid())
    let node = gId(this.pidid(rowNr))
    // console.log("Variable receiveValue", this, node, value, rowNr)

    if (node && value != null && node.childNodes) {
      // console.log("Coord3D receiveData", properties, node.childNodes);
      if (value.x && node.childNodes[0]) node.childNodes[0].value = value.x
      if (value.y && node.childNodes[1]) node.childNodes[1].value = value.y
      if (value.z && node.childNodes[2]) node.childNodes[2].value = value.z
    }
  }
  
  generateValue() {
    return `{"x":${Math.round(Math.random()*360)}, "y":${Math.round(Math.random()*360)}, "z":${Math.round(Math.random()*360)}}`
  }

} //class Coord3DVariable

