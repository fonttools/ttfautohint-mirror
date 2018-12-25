/*!
 * toc-unfold.js
 *
 * JavaScript code to fold and unfold TOC entries
 *
 * written 2014 by Werner Lemberg
 */

// This code snippet needs `jquery' (https://code.jquery.com/jquery-1.11.2.js)

$(document).ready(function() {
  TOC.setup();
});

var TOC = {
  setup: function() {
    /* decorate TOC's `li' elements that have children with `folded' class */
    /* (pandoc doesn't do this) */
    $("#TOC li").each(function(i) {
      if ($(this).children().length) {
        $(this).addClass("folded");
      }
    });

    /* fold all entries */
    $(".folded").each(function(i) {
      $(this).children("ul:first").hide();
    });

    /* change class to `unfolded' if clicked */
    $(".folded").click(function(e) {
      $(this).toggleClass("unfolded");
      $(this).children("ul:first").slideToggle("300");
      e.stopPropagation();
    });  
  }
}
