$( document ).ready(function() {

    $("div.headertitle").addClass("page-header");
    $("div.title").addClass("h1");

    $('li > a[href="index.html"] > span').before("<i class='fa fa-cog'></i> ");
    $('li > a[href="modules.html"] > span').before("<i class='fa fa-square'></i> ");
    $('li > a[href="namespaces.html"] > span').before("<i class='fa fa-bars'></i> ");
    $('li > a[href="annotated.html"] > span').before("<i class='fa fa-list-ul'></i> ");
    $('li > a[href="classes.html"] > span').before("<i class='fa fa-book'></i> ");
    $('li > a[href="inherits.html"] > span').before("<i class='fa fa-sitemap'></i> ");
    $('li > a[href="functions.html"] > span').before("<i class='fa fa-list'></i> ");
    $('li > a[href="functions_func.html"] > span').before("<i class='fa fa-list'></i> ");
    $('li > a[href="functions_vars.html"] > span').before("<i class='fa fa-list'></i> ");
    $('li > a[href="functions_enum.html"] > span').before("<i class='fa fa-list'></i> ");
    $('li > a[href="functions_eval.html"] > span').before("<i class='fa fa-list'></i> ");
    $('img[src="ftv2ns.png"]').replaceWith('<span class="label label-danger">N</span> ');
    $('img[src="ftv2cl.png"]').replaceWith('<span class="label label-danger">C</span> ');

    $("ul.tablist").addClass("nav nav-pills nav-justified");
    $("ul.tablist").css("margin-top", "0.5em");
    $("ul.tablist").css("margin-bottom", "0.5em");
    $("li.current").addClass("active");
    $("iframe").attr("scrolling", "yes");

    $("#nav-path > ul").addClass("breadcrumb");

    $("table.params").addClass("table");
    $("div.ingroups").wrapInner("<span class='text-nowrap'></span>");
    $("div.levels").css("margin", "0.5em");
    $("div.levels > span").addClass("btn btn-default btn-xs");
    $("div.levels > span").css("margin-right", "0.25em");

    $("table.directory").addClass("table table-striped");
    $("div.summary > a").addClass("btn btn-default btn-xs");
    $("table.fieldtable").addClass("table");
    $(".fragment").addClass("well");
    $(".memitem").addClass("panel panel-default");
    $(".memproto").addClass("panel-heading");
    $(".memdoc").addClass("panel-body");
    $("span.mlabel").addClass("label label-info");

    $("table.memberdecls").addClass("table");
    $("[class^=memitem]").addClass("active");

    $("div.ah").addClass("btn btn-default");
    $("span.mlabels").addClass("pull-right");
    $("table.mlabels").css("width", "100%")
    $("td.mlabels-right").addClass("pull-right");

    $("div.ttc").addClass("panel panel-primary");
    $("div.ttname").addClass("panel-heading");
    $("div.ttname a").css("color", 'white');
    $("div.ttdef,div.ttdoc,div.ttdeci").addClass("panel-body");

    $('div.fragment.well div.line:first').css('margin-top', '2px');
    $('div.fragment.well div.line:last').css('margin-bottom', '2px');

	$('table.doxtable').removeClass('doxtable').addClass('table table-striped table-bordered').each(function(){
		$(this).prepend('<thead></thead>');
		$(this).find('tbody > tr:first').prependTo($(this).find('thead'));

		$(this).find('td > span.success').parent().addClass('success');
		$(this).find('td > span.warning').parent().addClass('warning');
		$(this).find('td > span.danger').parent().addClass('danger');
	});

    // source code github links
    $('li:contains("FreeCAD/src/")').each(function(){
        var lk = '<a href="https://github.com/FreeCAD/FreeCAD/blob/master' + $(this).html().substr(7) + '">';
        lk += $(this).html();
        lk += '</a>';
        $(this).html(lk);
    });

    $(".memdoc pre.fragment").contents().unwrap();

    if($('div.fragment.well div.ttc').length > 0)
    {
        $('div.fragment.well div.line:first').parent().removeClass('fragment well');
    }

    $('table.memberdecls').find('.memItemRight').each(function(){
        $(this).contents().appendTo($(this).siblings('.memItemLeft'));
        $(this).siblings('.memItemLeft').attr('align', 'left');
    });

    $('table.memberdecls').find('.memTemplItemRight').each(function(){
        $(this).contents().appendTo($(this).siblings('.memTemplItemLeft'));
        $(this).siblings('.memTemplItemLeft').attr('align', 'left');
    });

	function getOriginalWidthOfImg(img_element) {
		var t = new Image();
		t.src = (img_element.getAttribute ? img_element.getAttribute("src") : false) || img_element.src;
		return t.width;
	}

	$('div.dyncontent').find('img').each(function(){
		if(getOriginalWidthOfImg($(this)[0]) > $('#content>div.container').width())
			$(this).css('width', '100%');
	});


  /* responsive search box */
  $('#MSearchBox').parent().remove();

  var nav_container = $('<div class="row"></div>');
  $('#navrow1').parent().prepend(nav_container);

  var left_nav = $('<div class="col-md-9"></div>');
  for (i = 0; i < 6; i++) {
    var navrow = $('#navrow' + i + ' > ul.tablist').detach();
    left_nav.append(navrow);
    $('#navrow' + i).remove();
  }
  var right_nav = $('<div class="col-md-3"></div>').append('\
    <div id="search-box" class="input-group">\
      <div class="input-group-btn">\
        <button aria-expanded="false" type="button" class="btn btn-default dropdown-toggle" data-toggle="dropdown">\
          <span class="glyphicon glyphicon-search"></span> <span class="caret"></span>\
        </button>\
        <ul class="dropdown-menu">\
        </ul>\
      </div>\
      <button id="search-close" type="button" class="close" aria-label="Close"><span aria-hidden="true">&times;</span></button>\
      <input id="search-field" class="form-control" accesskey="S" onkeydown="searchBox.OnSearchFieldChange(event);" placeholder="Search ..." type="text">\
    </div>');
  $(nav_container).append(left_nav);
  $(nav_container).append(right_nav);

  $('#MSearchSelectWindow .SelectionMark').remove();
  var search_selectors = $('#MSearchSelectWindow .SelectItem');
  for (var i = 0; i < search_selectors.length; i += 1) {
    var element_a = $('<a href="#"></a>').text($(search_selectors[i]).text());

    element_a.click(function(){
      $('#search-box .dropdown-menu li').removeClass('active');
      $(this).parent().addClass('active');
      searchBox.OnSelectItem($('#search-box li a').index(this));
      searchBox.Search();
      return false;
    });

    var element = $('<li></li>').append(element_a);
    $('#search-box .dropdown-menu').append(element);
  }
  $('#MSearchSelectWindow').remove();

  $('#search-box .close').click(function (){
    searchBox.CloseResultsWindow();
  });

  $('wrapper').append('<div id="MSearchClose"></div>');
  $('wrapper').append('<div id="MSearchBox"></div>');
  $('wrapper').append('<div id="MSearchSelectWindow"></div>');

  searchBox.searchLabel = '';
  searchBox.DOMSearchField = function() {
    return document.getElementById("search-field");
  }
  searchBox.DOMSearchClose = function(){
    return document.getElementById("search-close");
  }

  /* search results */
  var results_iframe = $('#MSearchResults').detach();
  $('#MSearchResultsWindow')
    .attr('id', 'search-results-window')
    .addClass('panel panel-default')
    .append(
      '<div class="panel-heading">\
        <h3 class="panel-title">Search Results</h3>\
      </div>\
      <div class="panel-body"></div>'
    );
  $('#search-results-window .panel-body').append(results_iframe);

  searchBox.DOMPopupSearchResultsWindow = function() {
    return document.getElementById("search-results-window");
  }

  function update_search_results_window() {
    $('#search-results-window').removeClass('panel-default panel-success panel-warning panel-danger')
    var status = $('#MSearchResults').contents().find('.SRStatus:visible');
    if (status.length > 0) {
      switch(status.attr('id')) {
        case 'Loading':
        case 'Searching':
          $('#search-results-window').addClass('panel-warning');
          break;
        case 'NoMatches':
          $('#search-results-window').addClass('panel-danger');
          break;
        default:
          $('#search-results-window').addClass('panel-default');
      }
    } else {
      $('#search-results-window').addClass('panel-success');
    }
  }
  $('#MSearchResults').load(function() {
    $('#MSearchResults').contents().find('link[href="search.css"]').attr('href','../doxygen.css');
    $('#MSearchResults').contents().find('head').append(
      '<link href="../customdoxygen.css" rel="stylesheet" type="text/css">');

    update_search_results_window();

    // detect status changes (only for search with external search backend)
    var observer = new MutationObserver(function(mutations) {
      update_search_results_window();
    });
    var config = { attributes: true};

    var targets = $('#MSearchResults').contents().find('.SRStatus');
    for (i = 0; i < targets.length; i++) {
      observer.observe(targets[i], config);
    }
  });


  /* enumerations */
  $('table.fieldtable').removeClass('fieldtable').addClass('table table-striped table-bordered').each(function(){
    $(this).prepend('<thead></thead>');
    $(this).find('tbody > tr:first').prependTo($(this).find('thead'));

    $(this).find('td > span.success').parent().addClass('success');
    $(this).find('td > span.warning').parent().addClass('warning');
    $(this).find('td > span.danger').parent().addClass('danger');
  });

  /* todo list */
  var todoelements = $('.contents > .textblock > dl.reflist > dt, .contents > .textblock > dl.reflist > dd');
  for (var i = 0; i < todoelements.length; i += 2) {
    $('.contents > .textblock').append(
      '<div class="panel panel-default active">'
        + "<div class=\"panel-heading todoname\">" + $(todoelements[i]).html() + "</div>"
        + "<div class=\"panel-body\">" + $(todoelements[i+1]).html() + "</div>"
      + '</div>');
  }
  $('.contents > .textblock > dl').remove();


	$(".memitem").removeClass('memitem');
    $(".memproto").removeClass('memproto');
    $(".memdoc").removeClass('memdoc');
	$("span.mlabel").removeClass('mlabel');
	$("table.memberdecls").removeClass('memberdecls');
    $("[class^=memitem]").removeClass('memitem');
    $("span.mlabels").removeClass('mlabels');
    $("table.mlabels").removeClass('mlabels');
    $("td.mlabels-right").removeClass('mlabels-right');
	$(".navpath").removeClass('navpath');
	$("li.navelem").removeClass('navelem');
	$("a.el").removeClass('el');
	$("div.ah").removeClass('ah');
	$("div.header").removeClass("header");

	$('.mdescLeft').each(function(){
		if($(this).html()=="&nbsp;") {
			$(this).siblings('.mdescRight').attr('colspan', 2);
			$(this).remove();
		}
	});
  $('td.memItemLeft').each(function(){
    if($(this).siblings('.memItemRight').html()=="") {
      $(this).attr('colspan', 2);
      $(this).siblings('.memItemRight').remove();
    }
  });
	$('td.memTemplItemLeft').each(function(){
		if($(this).siblings('.memTemplItemRight').html()=="") {
			$(this).attr('colspan', 2);
			$(this).siblings('.memTemplItemRight').remove();
		}
	});
  searchBox.CloseResultsWindow();
});

// contents of dynsections.js which seems to be missing from std doxygen js files...
function toggleVisibility(linkObj)
{
 var base = $(linkObj).attr('id');
 var summary = $('#'+base+'-summary');
 var content = $('#'+base+'-content');
 var trigger = $('#'+base+'-trigger');
 var src=$(trigger).attr('src');
 if (content.is(':visible')===true) {
   content.hide();
   summary.show();
   $(linkObj).addClass('closed').removeClass('opened');
   $(trigger).attr('src',src.substring(0,src.length-8)+'closed.png');
 } else {
   content.show();
   summary.hide();
   $(linkObj).removeClass('closed').addClass('opened');
   $(trigger).attr('src',src.substring(0,src.length-10)+'open.png');
 }
 return false;
}
function updateStripes()
{
  $('table.directory tr').
       removeClass('even').filter(':visible:even').addClass('even');
}
function toggleLevel(level)
{
  $('table.directory tr').each(function(){
    var l = this.id.split('_').length-1;
    var i = $('#img'+this.id.substring(3));
    var a = $('#arr'+this.id.substring(3));
    if (l<level+1) {
      i.attr('src','ftv2folderopen.png');
      a.attr('src','ftv2mnode.png');
      $(this).show();
    } else if (l==level+1) {
      i.attr('src','ftv2folderclosed.png');
      a.attr('src','ftv2pnode.png');
      $(this).show();
    } else {
      $(this).hide();
    }
  });
  updateStripes();
}
function toggleFolder(id)
{
  //The clicked row
  var currentRow = $('#row_'+id);
  var currentRowImages = currentRow.find("img");
  //All rows after the clicked row
  var rows = currentRow.nextAll("tr");
  //Only match elements AFTER this one (can't hide elements before)
  var childRows = rows.filter(function() {
    var re = new RegExp('^row_'+id+'\\d+_$', "i"); //only one sub
    return this.id.match(re);
  });
  //First row is visible we are HIDING
  if (childRows.filter(':first').is(':visible')===true) {
    currentRowImages.filter("[id^=arr]").attr('src', 'ftv2pnode.png');
    currentRowImages.filter("[id^=img]").attr('src', 'ftv2folderclosed.png');
    rows.filter("[id^=row_"+id+"]").hide();
  } else { //We are SHOWING
    //All sub images
    var childImages = childRows.find("img");
    var childImg = childImages.filter("[id^=img]");
    var childArr = childImages.filter("[id^=arr]");
    currentRow.find("[id^=arr]").attr('src', 'ftv2mnode.png'); //open row
    currentRow.find("[id^=img]").attr('src', 'ftv2folderopen.png'); //open row
    childImg.attr('src','ftv2folderclosed.png'); //children closed
    childArr.attr('src','ftv2pnode.png'); //children closed
    childRows.show(); //show all children
  }
  updateStripes();
}
function toggleInherit(id)
{
  var rows = $('tr.inherit.'+id);
  var img = $('tr.inherit_header.'+id+' img');
  var src = $(img).attr('src');
  if (rows.filter(':first').is(':visible')===true) {
    rows.css('display','none');
    $(img).attr('src',src.substring(0,src.length-8)+'closed.png');
  } else {
    rows.css('display','table-row'); // using show() causes jump in firefox
    $(img).attr('src',src.substring(0,src.length-10)+'open.png');
  }
}
