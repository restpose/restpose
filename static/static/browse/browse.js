function display_obj(obj) {
  var i, output, value;
  output = "<ul>";
  for (i in obj) {
    output += "<li>&quot;" + i + "&quot;";
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

function update_coll(target) {
  var data = target.data();
  function display_coll(obj) {
    var i, output, details;
    output = $("<li>").text(data.coll);
    details = $("<ul>");
    details.append($("<li>").text(obj.doc_count + " documents"));
    details.append($("<li>").text("types").append(display_obj(obj.types)));
    details.append($("<li>").text("patterns").append(display_obj(obj.patterns)));
    details.append($("<li>").text("special_fields").append(display_obj(obj.special_fields)));
    
    output.append(details);
    return output;
  }

  $.get("/coll/" + data.coll, function(result) {
    target.children().remove();
    target.append(display_coll(result));
  });
}

function update_colllist(target) {
  function display_colls(obj) {
    var i, output;
    output = $("<ul>");
    for (i in obj) {
      var item = $("<li><a/></li>");
      item.data("coll", i)
          .find("a")
          .text(i)
          .attr('href', '#coll/' + i)
          .click(function() { update_coll(item); });
      output.append(item);
    }
    return output.children();
  }

  $.get("/coll", function(result) {
    target.children().remove();
    target.append(display_colls(result));
  });
}
