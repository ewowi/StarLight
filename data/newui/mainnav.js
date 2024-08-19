// @title     StarBase
// @file      mainnav.js
// @date      20240819
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

/**
 * The main application class
 * Private pethods are prefixed with a '#' so they will be minimized by the bundler
 */
class MainNav {

  createHTML() {
    let body = document.getElementById("body");//gId("body");

    body.classList += "d-flex flex-column";

    body.innerHTML += `<!-- Alerts area sits at the top above the main navigation. Allow the height to expand vertically to fit any content 
       or shrink if there is nothing to show in the alerts area -->
  <div id="alerts" class="text-center bg-error">
    <!-- Place the alert text in a padded div so that when it's not present the alert area will be hidden-->
    <div class="pa-3 text-center">some alert text</div>
  </div>
  
  <!-- Main navigation bar has a fixed height and so it is always visible -->
  <div id="main-nav" class="d-flex align-stretch flex-shrink-0"></div>

  <!-- Main area contains the page content and fills vetically to fit the available broswer space. Overflow is set 
       to hidden so a vertical scrollbar never shows, allowing the footer to be fixed at the bottom -->
  <div id="main" class="flex-grow-1 overflow-hidden">
    <!-- Main content area has a secondary side navigation and an area for the active page -->
    <div id="main-content" class="d-flex h-100">
      <div id="second-nav" class="flex-shrink-0 overflow-y-auto">
      </div>
      <div id="page" class="flex-grow-1 flex-shrink-1 overflow-y-auto" style="word-break: break-word">

      </div>
    </div>

    <!-- This is an example of a page that overflows to scroll vertically, if you need that -->
    <!--
    <div id="main-content" class="h-100 overflow-y-auto">
      <h1 style="height:5em">Heading One</h1>
      <h1 style="height:5em">Heading Two</h1>
      <h1 style="height:5em">Heading Three</h1>
      <h1 style="height:5em">Heading Four</h1>
      <h1 style="height:5em">Heading Five</h1>
    </div>
    -->

    <!-- This is an example of a page that fills the entire context area without scrolling, if you need that -->
    <!--
    <div id="main-content" class="h-100 overflow-hidden" style="background-color: grey">
    -->
      <!-- Put a flex element in here with centered children to show that it is taking up the full width and height -->
      <!--
      <div class="d-flex h-100 align-center justify-center">
        <h1>Test Content Full Width + Height</h1>
      </div>
   </div>
    -->

  </div>
  
  <!-- Footer sits at the bottom with a fixed height -->
  <div id="footer" class="flex-shrink-0 text-center text-truncate pa-3">&copy; <span class='copy'></span> MoonModules ☾ - StarMod, StarBase and StarLight is licensed under GPL-v3</div>
`

    // Update the copyright notice in the footer
    document.querySelector('#footer .copy').innerText = new Date().getFullYear()
  } //addBody


  /**
   * Set the active module
   * @param {object} module - A module
   */
  set activeModuleJson(moduleJson) {
    if (!moduleJson) return

    this.#activeModuleJson = moduleJson

    localStorage.setItem('activeModuleId', moduleJson.id);

    // Update the main navigation
    document.querySelectorAll('#main-nav .menu-item').forEach(menuItem => {
      if (menuItem.dataset.type == this.#activeModuleJson.type) {
        menuItem.classList.add('selected')
      } else {
        menuItem.classList.remove('selected')
      }
    })

    // Update the secondary navigation
    document.querySelectorAll('#second-nav .menu-item').forEach(menuItem => {
      if (menuItem.dataset.id == this.#activeModuleJson.id) {
        menuItem.classList.add('selected')
      } else {
        menuItem.classList.remove('selected')
      }
    })

    // Update the page content
    if (this.#createHTMLFun) {
        document.getElementById('page').innerHTML = 
      `<div class="d-flex flex-column h-100 overflow-hidden">
        <div class="flex-shrink-0">
          <h1 class="title">${this.#activeModuleJson.id}</h1>
        </div>
        <div class="overflow-y-auto">` + this.#createHTMLFun(this.#activeModuleJson) +
        `</div>
      </div>`
    }
    //done after innerHTML as it needs to find the nodes. tbd: createHTMLFun adds to dom directly
    if (this.#setDefaultValuesFun) {
      this.#setDefaultValuesFun(this.#activeModuleJson)
    }
  } //set activeModuleJson(moduleJson)

  /**
   * Set the active module to the first one in the module list whose type matches
   * @param {string} moduleType - The module type
   * triggered by menu onclick
   */
  set activeModuleType(moduleType) {
    this.activeModuleJson = controller.modules.model.find(moduleJson => moduleJson.type == moduleType)
    this.#updateSecondaryMenu();
  }

  /**
   * Set the active module to the module specified by the id
   * @param {string} id - The module id
   * triggered by menu onclick
   */
  set activeModuleId(id) {
    this.activeModuleJson = controller.modules.model.find(moduleJson => moduleJson.id == id)
  }

  // Get the unique list of module types
  #updateMainMenu() {
    const uniqueTypes = controller.modules.model
    .filter((moduleJson, index, modules) => {
      return modules.findIndex(m => m.type === moduleJson.type) === index
    })
    .map(moduleJson => moduleJson.type)

    // Each module type becomes a top level menu
    const html = uniqueTypes
      .map(type => {
        const selected = type == this.#activeModuleJson.type ? 'selected' : ''
        return `<div onclick="controller.mainNav.activeModuleType=this.dataset.type" class="menu-item selectable ${selected} pa-4" data-type="${type}">${type}</div>`
      })
      .join('')
    document.getElementById('main-nav').innerHTML = html
  }

  // Update the secondary navigation menu
  #updateSecondaryMenu() {
    const modules = controller.modules.model.filter(moduleJson => moduleJson.type == this.#activeModuleJson.type)

    const html = modules
    .map(moduleJson => {
      const selected = moduleJson.id == this.#activeModuleJson.id ? 'selected' : ''
      return `<div onclick="controller.mainNav.activeModuleId=this.dataset.id" class="menu-item selectable ${selected} text-truncate pa-3" data-id="${moduleJson.id}">${moduleJson.id}</div>`
    })
    .join('')
    document.getElementById('second-nav').innerHTML = html
  }

  /**
   * Update the UI
   */
  //updateUI is made after all modules have been fetched, how to adapt to add one module?
  updateUI(moduleJson, createHTMLFun, setDefaultValuesFun) {

    //functions called when selecting a module
    this.#createHTMLFun = createHTMLFun
    this.#setDefaultValuesFun = setDefaultValuesFun

    this.#updateMainMenu();
    this.#updateSecondaryMenu();
  
    let activeModuleId = localStorage.getItem('activeModuleId');
    if (moduleJson.id == activeModuleId) {
      this.activeModuleJson = moduleJson //set triggered
    }

    // If there is no active module set it to the first one
    if (activeModuleId == null && this.#activeModuleJson == '' && controller.modules.model.length) {
      this.activeModuleJson = controller.modules.model[0]
    }
  }

  /**
   * Store the currently active module
   */
  #activeModuleJson = ''

  //stores the function to execute to display one module
  #createHTMLFun = null;
  #setDefaultValuesFun = null;
}