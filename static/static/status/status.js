function display_obj(obj) {
  var i, output, value;
  output = "<ul>";
  for (i in obj) {
    output += "<li>" + i;
    value = obj[i];
    if (typeof value == "object") {
      output += display_obj(value);
    } else {
      output += ": " + value;
    }
    output += "</li>";
  }
  output += "</ul>";
  return output;
}

function update_status(target_tasks) {
  $.get("/status", function(result) {
    target_tasks.html(display_obj(result.tasks));

    setTimeout(function() { update_status(target_tasks) }, 2000);
  });
}
