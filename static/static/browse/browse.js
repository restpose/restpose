$(function() {
  $("#collections").each(function() {
    $.get("/colls", function() {
      console.log(this);
    });
  });
});
