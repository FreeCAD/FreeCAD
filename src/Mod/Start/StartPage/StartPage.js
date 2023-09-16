var allowDownloads = 0;
var showForum = 0;
var wblist = [];

function toggle(tab)
{

    // switch to the given tab ID ("tab1", "tab2", etc...)

    var tabs = document.getElementById("tabs").childElementCount;
    document.getElementById(tab).classList.remove("hidden");
    document.getElementById("h" + tab).classList.add("active");
    for (var i = 1; i < tabs; i++) {
        if ("tab" + i != tab) {
            document.getElementById("tab" + i).classList.add("hidden");
            document.getElementById("htab" + i).classList.remove("active");
        }
    }
}


function load()
{

    // run at startup

    if (localStorage["notepad"]) {
        document.getElementById("notepad").value =
            localStorage["notepad"];  // Load notepad from local storage
    }
    document.getElementById("notepad").addEventListener("input", function() {
        localStorage.setItem("notepad",
                             document.getElementById("notepad").value);  // Save notepad on type
    }, false);

    if (allowDownloads == 1) {
        // load latest commits
        var ddiv = document.getElementById("commits");
        ddiv.innerHTML = "Connecting...";
        var tobj = new JSONscriptRequest(
            'https://api.github.com/repos/FreeCAD/FreeCAD/commits?callback=printCommits');
        tobj.buildScriptTag();  // Build the script tag
        tobj.addScriptTag();    // Execute (add) the script tag
        ddiv.innerHTML = "Downloading latest news...";
        // load addons list
        ddiv = document.getElementById("addons");
        ddiv.innerHTML = "Connecting...";
        var tobj = new JSONscriptRequest(
            'https://api.github.com/repos/FreeCAD/FreeCAD-addons/contents?callback=printAddons');
        tobj.buildScriptTag();  // Build the script tag
        tobj.addScriptTag();    // Execute (add) the script tag
        ddiv.innerHTML = "Downloading addons list...";
        if (showForum == 1) {
            // load forum recent posts
            ddiv = document.getElementById("forum");
            ddiv.innerHTML = "Connecting...";
            var tobj = new JSONscriptRequest(
                'https://www.freecad.org/xml-to-json.php?callback=printForum&url=https://forum.freecad.org/feed.php');
            tobj.buildScriptTag();  // Build the script tag
            tobj.addScriptTag();    // Execute (add) the script tag
            ddiv.innerHTML = "Downloading addons list...";
        }
    }
}


function printCommits(data)
{

    // json callback for git commits

    var ddiv = document.getElementById('commits');
    ddiv.innerHTML = "Received";
    var html = ['<ul>'];
    for (var i = 0; i < 25; i++) {
        html.push('<li><a href="',
                  data.data[i].html_url,
                  '">',
                  data.data[i].commit.message,
                  '</a> ',
                  data.data[i].commit.committer.date.split("T")[0],
                  ' - ',
                  data.data[i].commit.author.name,
                  '</li>');
    }
    html.push('</ul>');
    ddiv.innerHTML = html.join('');
}


function printAddons(data)
{

    // json callback for addons list

    var ddiv = document.getElementById('addons');
    ddiv.innerHTML = "Received";
    var html = ['<ul class="addonslist">'];
    var blacklist = ['addons_installer.FCMacro', 'FreeCAD-Addon-Details.md', 'README.md'];
    for (var i = 0; i < data.data.length; i++) {
        if ((data.data[i].name[0] != ".") && (blacklist.indexOf(data.data[i].name) < 0)) {
            if (wblist.indexOf(data.data[i].name.toLowerCase()) == -1) {
                html.push('<li><a href="',
                          data.data[i].html_url,
                          '">',
                          data.data[i].name,
                          '</a></li>');
            }
            else {
                html.push('<li><a href="',
                          data.data[i].html_url,
                          '">',
                          data.data[i].name,
                          '</a>&nbsp;<img src="IMAGE_SRC_INSTALLED"></li>');
            }
        }
    }
    html.push('</ul>');
    ddiv.innerHTML = html.join('');
}


function printForum(data)
{

    // json callback for forum posts

    var ddiv = document.getElementById('forum');
    ddiv.innerHTML = "Received";
    var html = ['<ul>'];
    for (var i = 0; i < 25; i++) {
        if (i < data.feed.entry.length) {
            html.push('<li><big><a href="',
                      data.feed.entry[i].link.href,
                      '">',
                      data.feed.entry[i].title.$,
                      '</a></big><br/><p>',
                      data.feed.entry[i].content.$,
                      '</p></li>');
        }
    }
    html.push('</ul>');
    ddiv.innerHTML = html.join('');
}


// below are JSON helper functions


function JSONscriptRequest(fullUrl)
{

    // REST request path
    this.fullUrl = fullUrl;
    // Get the DOM location to put the script tag
    this.headLoc = document.getElementsByTagName("head").item(0);
    // Generate a unique script tag id
    this.scriptId = 'JscriptId' + JSONscriptRequest.scriptCounter++;
}


// Static script ID counter
JSONscriptRequest.scriptCounter = 1;


JSONscriptRequest.prototype.buildScriptTag =
    function() {
    // Create the script tag
    this.scriptObj = document.createElement("script");
    // Add script object attributes
    this.scriptObj.setAttribute("type", "text/javascript");
    this.scriptObj.setAttribute("charset", "utf-8");
    this.scriptObj.setAttribute("src", this.fullUrl);
    this.scriptObj.setAttribute("id", this.scriptId);
}


    JSONscriptRequest.prototype.removeScriptTag =
        function() {
    // Destroy the script tag
    this.headLoc.removeChild(this.scriptObj);
}

        JSONscriptRequest.prototype.addScriptTag = function() {
    // Create the script tag
    this.headLoc.appendChild(this.scriptObj);
}
