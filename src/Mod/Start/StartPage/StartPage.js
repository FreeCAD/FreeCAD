var linkDescriptions = [];
var allowDownloads = 0;

function JSONscriptRequest(fullUrl) {
    // REST request path
    this.fullUrl = fullUrl; 
    // Get the DOM location to put the script tag
    this.headLoc = document.getElementsByTagName("head").item(0);
    // Generate a unique script tag id
    this.scriptId = 'JscriptId' + JSONscriptRequest.scriptCounter++;
}

// Static script ID counter
JSONscriptRequest.scriptCounter = 1;

JSONscriptRequest.prototype.buildScriptTag = function () {
    // Create the script tag
    this.scriptObj = document.createElement("script");
    // Add script object attributes
    this.scriptObj.setAttribute("type", "text/javascript");
    this.scriptObj.setAttribute("charset", "utf-8");
    this.scriptObj.setAttribute("src", this.fullUrl);
    this.scriptObj.setAttribute("id", this.scriptId);
}

JSONscriptRequest.prototype.removeScriptTag = function () {
    // Destroy the script tag
    this.headLoc.removeChild(this.scriptObj);  
}

JSONscriptRequest.prototype.addScriptTag = function () {
    // Create the script tag
    this.headLoc.appendChild(this.scriptObj);
}

function show(theText) {
    ddiv = document.getElementById("description");
    if (theText == "") theText = "&nbsp;";
    ddiv.innerHTML = theText;
}

function checkVersion(data) {
    vdiv = document.getElementById("versionbox");
    console.log('test');
    var cmajor = vmajor;
    var cminor = vminor;
    var cbuild = vbuild;
    var amajor = data[0]['major'];
    var aminor = data[0]['minor'];
    var abuild = data[0]['build'];
    if (cmajor >= amajor && cminor >= aminor && cbuild >= abuild) {
        vdiv.innerHTML=" text58: vmajor.vminor.vbuild";
    } else {
        vdiv.innerHTML="<a href=exthttp://github.com/FreeCAD/FreeCAD/releases/latest>text59:"+amajor+"."+aminor+"."+abuild+"</a>";
    }
}

function load() {
    ddiv = document.getElementById("news");
    if (allowDownloads == 0) {
        ddiv.innerHTML = '<a href="EnableDownload.py" onClick="enableDownloads()">text70</a> text71';
        return;
    }
    // load latest news
    ddiv.innerHTML = "Connecting...";
    var tobj=new JSONscriptRequest('https://api.github.com/repos/FreeCAD/FreeCAD/commits?callback=showTweets');
    tobj.buildScriptTag(); // Build the script tag
    tobj.addScriptTag(); // Execute (add) the script tag
    ddiv.innerHTML = "Downloading latest news...";

    // load version
    var script = document.createElement('script');
    script.src = 'http://www.freecadweb.org/version.php?callback=checkVersion';
    document.body.appendChild(script);
}

function stripTags(text) {
    // from http://www.pagecolumn.com/tool/all_about_html_tags.htm /<\s*\/?\s*span\s*.*?>/g
    stripped = text.replace("<table", "<div");
    stripped = stripped.replace("</table", "</div");
    stripped = stripped.replace("<tr", "<tr");
    stripped = stripped.replace("</tr", "</tr");
    stripped = stripped.replace("<td", "<td");
    stripped = stripped.replace("</td", "</td");
    stripped = stripped.replace("555px", "auto");
    stripped = stripped.replace("border:1px", "border:0px");
    stripped = stripped.replace("color:#000000;","");
    return stripped;
}

function showTweets(data) {
    ddiv = document.getElementById('news');
    if (allowDownloads == 0) {
        ddiv.innerHTML = '<a href="EnableDownload.py" onClick="enableDownloads()">text70</a> text71';
        return;
    }
    ddiv.innerHTML = "Received";
    var html = ['<ul>'];
    for (var i = 0; i < 4; i++) {
        html.push('<li><img src="images/web.png">&nbsp;<a href="ext', data.data[i].commit.url, '" onMouseOver="showDescr(', i+1, ')" onMouseOut="showDescr()">', data.data[i].commit.message, '</a></li>');
        if ("message" in data.data[i].commit) {
            linkDescriptions.push(stripTags(data.data[i].commit.message)+'<br/>'+data.data[i].commit.author.name+'<br/>'+data.data[i].commit.author.date);
        } else {
            linkDescriptions.push("");
        }
        
    }
    html.push('</ul>');
    html.push('<a href="exthttp://github.com/FreeCAD/FreeCAD/commits/master">text63<a/>');
    ddiv.innerHTML = html.join('');
    resize();
    resize(); // intentional javascript hack
}

function enableDownloads() {
    allowDownloads = 1;
    // load latest news
    ddiv = document.getElementById("news");
    ddiv.innerHTML = "Connecting...";
    var tobj=new JSONscriptRequest('https://api.github.com/repos/FreeCAD/FreeCAD/commits?callback=showTweets');
    tobj.buildScriptTag(); // Build the script tag
    tobj.addScriptTag(); // Execute (add) the script tag
    ddiv.innerHTML = "Downloading latest news...";
}

function showDescr(d) {
    if (d) {
        show(linkDescriptions[d-1]);
    } else {
        show("");
    }
}

function scroller() {
    desc = document.getElementById("description");
    base = document.getElementById("column").offsetTop;
    scro = window.scrollY;
    if (scro > base) {
        desc.className = "stick";
    } else {
        desc.className = "";
    }
}

function resize() {
    var halfblocks = document.getElementsByClassName('half-block');
    var commits = document.getElementById("commits");
    var examples = document.getElementById("examples");
    var files = document.getElementById("files");
    var newproj = document.getElementById('newproject');
    var ontheweb = document.getElementById('ontheweb');
    var description = document.getElementById("description");

    var w = files.offsetWidth;
    var filesHeight = files.offsetHeight;
    var h1 = newproj.getElementsByTagName('ul')[0].offsetHeight;
    var h2 = ontheweb.getElementsByTagName('ul')[0].offsetHeight;
    var h3 = examples.getElementsByTagName('ul')[0].offsetHeight;
    var h4 = commits.getElementsByTagName('ul')[0].offsetHeight;

    var rowTwoHeight = h1 > h2 ? h1 : h2;
    var rowThreeHeight = h3 > h4 ? h3 : h4;
    rowTwoHeight = rowTwoHeight + 40;
    rowThreeHeight = rowThreeHeight + 80;
    var totalHeight = rowTwoHeight + rowThreeHeight + filesHeight + 36;

    w = w - 45;
    for (var i = 0; i < halfblocks.length; i++) {
        halfblocks[i].style.width = w/2 + 'px';
    }
    newproj.style.height = rowTwoHeight + 'px';
    ontheweb.style.height = rowTwoHeight + 'px';
    examples.style.height = rowThreeHeight + 'px';
    commits.style.height =  rowThreeHeight + 'px';
    description.style.height = totalHeight + 'px';
}

document.onmousemove=scroller;
