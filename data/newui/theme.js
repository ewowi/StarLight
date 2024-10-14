// @title     StarBase
// @file      theme.js
// @date      20241014
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com


class Theme {

  createHTML() {
    let body = gId("body");

    body.innerHTML += `<p><label>Theme</label> 
    <select name="theme-select" id="theme-select" onchange="controller.theme.setTheme(this.value)">
      <option value="starbase">StarBase</option>
      <option value="starlight">StarLight</option>
      <option value="wled">WLED</option>
      <option value="grayeen">Grayeen</option>
      <option value="dev">Dev</option>
      <option value="light">Light</option>
      <option value="dark">DeathStar</option>
      <option value="blue">Blue</option>
      <option value="pink">Pink</option>
      <option value="space">Space</option>
      <option value="nyan">Nyan</option>
    </select></p>`
  }

  setTheme(value) {
    console.log("setTheme", value)
    localStorage.setItem('theme', value);
    document.documentElement.className = value;
    // if (gId("theme-select").value != value)
    //   gId("theme-select").value = value;
  }

  getTheme() {
    let value = localStorage.getItem('theme');
    // console.log("getTheme", value, gId("theme-select"))
    if (value && value != "null") 
      gId("theme-select").value = value
  }

}