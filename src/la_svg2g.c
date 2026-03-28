#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "line.h"
#include "gcode.h"
#include "svg_transform.h"
#include "svg_path.h"

float max_error = 0.05f;

// Lines array
int lines_cap = 100;
int lines_count = 0;
Line *lines = NULL;

// Helper: Add a line to the array with transformation applied
void add_line(float x0, float y0, float x1, float y1, Matrix m) {
    if (lines_count >= lines_cap) {
        lines_cap *= 2;
        lines = realloc(lines, lines_cap * sizeof(Line));
    }
    
    Line line = {x0, y0, x1, y1, 100.0f, 100.0f};
    transform_line(&line, m);
    lines[lines_count++] = line;
}

// Get element name
const char* getname(xmlNodePtr node) {
    return (const char*)node->name;
}

// Get string attribute (returns empty string if not found)
const char* getattrs(xmlNodePtr node, const char *attr) {
    xmlChar *val = xmlGetProp(node, (const xmlChar*)attr);
    if (!val) return "";
    static char buf[256];
    snprintf(buf, sizeof(buf), "%s", (const char*)val);
    xmlFree(val);
    return buf;
}

// Get float attribute (returns 0 if not found)
float getattrf(xmlNodePtr node, const char *attr) {
    const char *val = getattrs(node, attr);
    return (*val) ? atof(val) : 0.0f;
}

// Parse transform attribute and return resulting matrix
// Handles multiple transforms: transform="translate(10,20) rotate(45)"
Matrix gettransform(xmlNodePtr node) {
    const char *transform_str = getattrs(node, "transform");
    return parse_transform_string(transform_str);
}

// Get stroke color from style attribute
const char* getcolor(xmlNodePtr node) {
    const char *style = getattrs(node, "style");
    static char color[32];
    
    const char *stroke = strstr(style, "stroke:");
    if (stroke) {
        sscanf(stroke, "stroke:%31s", color);
        // Clean up trailing semicolon or space
        char *end = strchr(color, ';');
        if (end) *end = '\0';
        return color;
    }
    return "black";
}

void handle_rect(xmlNodePtr node, Matrix transform) {
    float x = getattrf(node, "x");
    float y = getattrf(node, "y");
    float w = getattrf(node, "width");
    float h = getattrf(node, "height");
    float rx = getattrf(node, "rx");
    float ry = getattrf(node, "ry");

    // If only one radius specified, use it for both
    if (rx == 0) rx = ry;
    if (ry == 0) ry = rx;

    // Clamp radii to half the rectangle dimensions
    rx = fminf(rx, w * 0.5f);
    ry = fminf(ry, h * 0.5f);

    // If no rounding, draw simple rectangle
    if (rx == 0 || ry == 0) {
        // Top edge
        plot_line(x + w, y, transform);
        // Right edge
        plot_line(x + w, y + h, transform);
        // Bottom edge
        plot_line(x, y + h, transform);
        // Left edge
        plot_line(x, y, transform);
        return;
    }

    // Draw rounded rectangle using arcs for corners
    // Top-left corner arc
    path_x = x + rx;
    path_y = y;
    plot_arc(rx, ry, 0, 0, 0, x, y + ry, transform, max_error);
    // Left edge
    plot_line(x, y + h - ry, transform);
    // Bottom-left corner arc
    plot_arc(rx, ry, 0, 0, 0, x + rx, y + h, transform, max_error);
    // Bottom edge
    plot_line(x + w - rx, y + h, transform);
    // Bottom-right corner arc
    plot_arc(rx, ry, 0, 0, 0, x + w, y + h - ry, transform, max_error);
    // Right edge
    plot_line(x + w, y + ry, transform);
    // Top-right corner arc
    plot_arc(rx, ry, 0, 0, 0, x + w - rx, y, transform, max_error);
    // Top edge
    plot_line(x + rx, y, transform);
}

void handle_circle(xmlNodePtr node, Matrix transform) {
    float cx = getattrf(node, "cx");
    float cy = getattrf(node, "cy");
    float r = getattrf(node, "r");

    float k = BEZIER_CIRCLE_K * r;

    // Start at rightmost point
    path_x = cx + r;
    path_y = cy;

    // Top-right quadrant
    plot_cubic_bezier(cx + r, cy + k, cx + k, cy + r, cx, cy + r, transform, max_error);
    // Top-left quadrant
    plot_cubic_bezier(cx - k, cy + r, cx - r, cy + k, cx - r, cy, transform, max_error);
    // Bottom-left quadrant
    plot_cubic_bezier(cx - r, cy - k, cx - k, cy - r, cx, cy - r, transform, max_error);
    // Bottom-right quadrant
    plot_cubic_bezier(cx + k, cy - r, cx + r, cy - k, cx + r, cy, transform, max_error);
}

void handle_ellipse(xmlNodePtr node, Matrix transform) {
    float cx = getattrf(node, "cx");
    float cy = getattrf(node, "cy");
    float rx = getattrf(node, "rx");
    float ry = getattrf(node, "ry");

    float kx = BEZIER_CIRCLE_K * rx;
    float ky = BEZIER_CIRCLE_K * ry;

    // Start at rightmost point
    path_x = cx + rx;
    path_y = cy;

    // Top-right quadrant
    plot_cubic_bezier(cx + rx, cy + ky, cx + kx, cy + ry, cx, cy + ry, transform, max_error);
    // Top-left quadrant
    plot_cubic_bezier(cx - kx, cy + ry, cx - rx, cy + ky, cx - rx, cy, transform, max_error);
    // Bottom-left quadrant
    plot_cubic_bezier(cx - rx, cy - ky, cx - kx, cy - ry, cx, cy - ry, transform, max_error);
    // Bottom-right quadrant
    plot_cubic_bezier(cx + kx, cy - ry, cx + rx, cy - ky, cx + rx, cy, transform, max_error);
}


// Parse line element
void handle_line(xmlNodePtr node, Matrix transform) {
    float x1 = getattrf(node, "x1");
    float y1 = getattrf(node, "y1");
    float x2 = getattrf(node, "x2");
    float y2 = getattrf(node, "y2");
    
    add_line(x1, y1, x2, y2, transform);
}

// Recursive tree walker
void walk_tree(xmlNodePtr node, Matrix transform) {
    if (!node) return;
    
    // Apply this node's transform to the current matrix
    Matrix node_transform = gettransform(node);
    Matrix combined = matrix_multiply(transform, node_transform);

    const char *name = getname(node);
//fprintf(stderr, "[la_svg2g] Now in node: %s\n", name);

    // Check if this element has a path
    // Some SVG writers do stuff like a circle with path data instead of dimensions
    // For such cases, we prefer path data whenever available
    const char *path_data = getpath(node);
    if (*path_data) {
        parse_path_data(path_data, combined, max_error);
        return; // Path elements don't have children to process
    }
    
    // Branch on element type
    if (strcmp(name, "rect") == 0) {
        handle_rect(node, combined);
    } else if (strcmp(name, "circle") == 0) {
        handle_circle(node, combined);
    } else if (strcmp(name, "ellipse") == 0) {
        handle_ellipse(node, combined);
    } else if (strcmp(name, "line") == 0) {
        handle_line(node, combined);
    } else if (strcmp(name, "g") == 0 || strcmp(name, "svg") == 0) {
        // Group: recurse into children with combined transform
        for (xmlNodePtr child = node->children; child; child = child->next) {
            if (child->type == XML_ELEMENT_NODE) {
                walk_tree(child, combined);
            }
        }
    }
    // Ignore all other elements
}

// Entry point: parse SVG from stdin
void parse_svg() {
    lines = malloc(lines_cap * sizeof(Line));
    
    xmlDocPtr doc = xmlParseFile("-"); // stdin
    if (!doc) {
        fprintf(stderr, "[la_svg2g] Failed to parse SVG\n");
        return;
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc);
    Matrix identity = {1, 0, 0, 1, 0, 0};
    walk_tree(root, identity);
    
    xmlFreeDoc(doc);
}

float parse_float_arg(int argc, char *argv[], const char *flag, float default_val) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return atof(argv[i + 1]);
        }
    }
    return default_val;
}

int main(int argc, char *argv[]) {

    float s = parse_float_arg(argc, argv, "-s", 100.0f);
    float f = parse_float_arg(argc, argv, "-f", 100.0f);
    fprintf(stderr, "[la_svg2g] Using S=%g and F=%g.\n", s, f);

    parse_svg();
    fprintf(stderr, "[la_svg2g] Parsed %d cut lines from the input SVG file.\n", lines_count);

    for (int i = 0; i < lines_count; i++) {
        lines[i].s = s;
        lines[i].f = f;
    }
    export_gcode(lines, lines_count);
    free(lines);
    return 0;
}

