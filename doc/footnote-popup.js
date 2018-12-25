/*!
 * footnote-popup.js
 *
 * JavaScript code to make pandoc footnotes pop up
 *
 * written 2014 by Werner Lemberg
 * based on code from http://ignorethecode.net/blog/2010/04/20/footnotes
 */

// This code snippet needs `jquery' (https://code.jquery.com/jquery-1.11.2.js)

// Add a #footnotediv rule to the CSS code to style the pop-up window.

$(document).ready(function() {
  Footnotes.setup();
});

var Footnotes = {
  footnotetimeout: false,

  setup: function() {
    /* collect all footnote links that have an ID starting with `fnref' */
    var footnotelinks = $("a[id^='fnref']")

    /* clean-up */
    footnotelinks.unbind('mouseover', Footnotes.footnoteover);
    footnotelinks.unbind('mouseout', Footnotes.footnoteoout);

    /* assign our mouse handling functions to the collected footnote links */
    footnotelinks.bind('mouseover', Footnotes.footnoteover);
    footnotelinks.bind('mouseout', Footnotes.footnoteoout);
  },

  footnoteover: function() {
    clearTimeout(Footnotes.footnotetimeout);

    /* clean-up */
    $('#footnotediv').stop();
    $('#footnotediv').remove();

    /* extract position of the current footnote link and get the ID the
       `href' attribute is pointing to */
    var id = $(this).attr('href').substr(1);
    var position = $(this).offset();

    /* build a diversion having the ID `footnotediv' */
    var div = $(document.createElement('div'));
    div.attr('id', 'footnotediv');

    /* assign our mouse handling functions to the diversion */
    div.bind('mouseover', Footnotes.divover);
    div.bind('mouseout', Footnotes.footnoteoout);

    /* get the footnote data */
    var el = document.getElementById(id);
    div.html($(el).html());

    /* pandoc inserts a `return symbol' (embedded in a hyperlink) at the
       very end of the footnote; it doesn't make sense to display it in a
       pop-up window, so we remove it */
    div.find('a').last().remove();

    /* finally, create some CSS data for the diversion */
    div.css({
      position: 'absolute',
      width: '20em',
      opacity: 0.95
    });
    $(document.body).append(div);

    /* ensure that our pop-up window gets displayed on screen */
    var left = position.left;
    if (left + div.width() + 20 > $(window).width() + $(window).scrollLeft())
      left = $(window).width() - div.width() - 20 + $(window).scrollLeft();

    var top = position.top + 20;
    if (top + div.height() > $(window).height() + $(window).scrollTop())
      top = position.top - div.height() - 15;

    div.css({
      left: left,
      top: top
    });
  },

  footnoteoout: function() {
    /* as soon as the mouse cursor leaves the diversion, fade out and remove
       it eventually */
    Footnotes.footnotetimeout = setTimeout(function() {
        $('#footnotediv').animate({ opacity: 0 },
                                  600,
                                  function() {
                                    $('#footnotediv').remove();
                                  });
      },
      100);
  },

  divover: function() {
    clearTimeout(Footnotes.footnotetimeout);

    /* as soon as the mouse cursor is over the diversion (again), stop a
       previous animation and make it immediately visible */
    $('#footnotediv').stop();
    $('#footnotediv').css({ opacity: 0.95 });
  }
}
