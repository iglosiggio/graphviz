#include <stdexcept>

#include <fmt/format.h>

#include "svg_element.h"
#include <cgraph/unreachable.h>

SVG::SVGElement::SVGElement(SVGElementType type) : type(type) {}

static double px_to_pt(double px) {
  // a `pt` is 0.75 `px`. See e.g.
  // https://oreillymedia.github.io/Using_SVG/guide/units.html
  return px * 3 / 4;
}

static std::string xml_encode(const std::string &text) {
  std::string out;
  for (const char &ch : text) {
    switch (ch) {
    case '>':
      out += "&gt;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '-':
      out += "&#45;";
      break;
    case '&':
      out += "&amp;";
      break;
    default:
      out += ch;
    }
  }
  return out;
}

void SVG::SVGElement::append_attribute(std::string &output,
                                       const std::string &attribute) const {
  if (attribute.empty()) {
    return;
  }
  if (!output.empty()) {
    output += " ";
  }
  output += attribute;
}

std::string SVG::SVGElement::id_attribute_to_string() const {
  if (attributes.id.empty()) {
    return "";
  }

  return fmt::format(R"(id="{}")", attributes.id);
}

std::string SVG::SVGElement::to_string(std::size_t indent_size = 2) const {
  std::string output;
  output += R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>)"
            "\n";
  output += R"(<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN")"
            "\n";
  output += R"( "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">)"
            "\n";
  output += fmt::format("<!-- Generated by graphviz version {} "
                        "({})\n -->\n",
                        graphviz_version, graphviz_build_date);
  to_string_impl(output, indent_size, 0);
  return output;
}

void SVG::SVGElement::to_string_impl(std::string &output,
                                     std::size_t indent_size,
                                     std::size_t current_indent) const {
  const auto indent_str = std::string(current_indent, ' ');
  output += indent_str;

  if (type == SVG::SVGElementType::Svg) {
    const auto comment = fmt::format("Title: {} Pages: 1", graphviz_id);
    output += fmt::format("<!-- {} -->\n", xml_encode(comment));
  }
  if (type == SVG::SVGElementType::Group &&
      (attributes.class_ == "node" || attributes.class_ == "edge")) {
    const auto comment = graphviz_id;
    output += fmt::format("<!-- {} -->\n", xml_encode(comment));
  }

  output += "<";
  output += tag(type);

  std::string attributes_str{};
  append_attribute(attributes_str, id_attribute_to_string());
  switch (type) {
  case SVG::SVGElementType::Ellipse:
    // ignore for now
    break;
  case SVG::SVGElementType::Group:
    attributes_str += fmt::format(R"( class="{}")", attributes.class_);
    if (attributes.transform.has_value()) {
      const auto transform = attributes.transform;
      attributes_str += fmt::format(
          R"|( transform="scale({} {}) rotate({}) translate({} {})")|",
          transform->a, transform->d, transform->c, transform->e, transform->f);
    }
    break;
  case SVG::SVGElementType::Path:
    // ignore for now
    break;
  case SVG::SVGElementType::Polygon:
    // ignore for now
    break;
  case SVG::SVGElementType::Polyline:
    // ignore for now
    break;
  case SVG::SVGElementType::Svg:
    attributes_str += fmt::format(
        R"(width="{}pt" height="{}pt")"
        "\n"
        R"( viewBox="{:.2f} {:.2f} {:.2f} {:.2f}" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink")",
        std::lround(px_to_pt(attributes.width)),
        std::lround(px_to_pt(attributes.height)), attributes.viewBox.x,
        attributes.viewBox.y, attributes.viewBox.width,
        attributes.viewBox.height);
    break;
  case SVG::SVGElementType::Text:
    // ignore for now
    break;
  case SVG::SVGElementType::Title:
    // Graphviz doesn't generate attributes on 'title' elements
    break;
  default:
    throw std::runtime_error{fmt::format(
        "Attributes on '{}' elements are not yet implemented", tag(type))};
  }
  if (!attributes_str.empty()) {
    output += " ";
  }
  output += attributes_str;

  if (children.empty() && text.empty()) {
    output += "/>\n";
  } else {
    output += ">";
    if (!text.empty()) {
      output += xml_encode(text);
    }
    if (!children.empty()) {
      output += "\n";
      for (const auto &child : children) {
        child.to_string_impl(output, indent_size, current_indent + indent_size);
      }
      output += indent_str;
    }
    output += "</";
    output += tag(type);
    output += ">\n";
  }
}

std::string_view SVG::tag(SVGElementType type) {
  switch (type) {
  case SVG::SVGElementType::Circle:
    return "circle";
  case SVG::SVGElementType::Ellipse:
    return "ellipse";
  case SVG::SVGElementType::Group:
    return "g";
  case SVG::SVGElementType::Line:
    return "line";
  case SVG::SVGElementType::Path:
    return "path";
  case SVG::SVGElementType::Polygon:
    return "polygon";
  case SVG::SVGElementType::Polyline:
    return "polyline";
  case SVG::SVGElementType::Rect:
    return "rect";
  case SVG::SVGElementType::Svg:
    return "svg";
  case SVG::SVGElementType::Text:
    return "text";
  case SVG::SVGElementType::Title:
    return "title";
  }
  UNREACHABLE();
}
