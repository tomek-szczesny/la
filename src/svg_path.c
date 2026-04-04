#include "svg_path.h"

// Current position in path
static float path_x = 0.0f;
static float path_y = 0.0f;
    
// Tokenizer state
typedef struct {
    const char *str;
    size_t pos;
} PathTokenizer;

static PathTokenizer tokenizer;

// Get path data attribute (handles arbitrary length)
const char* getpath(xmlNodePtr node) {
    xmlChar *val = xmlGetProp(node, (const xmlChar*)"d");
    if (!val) return "";
    
    // Store in static buffer (caller must use immediately)
    static char *buf = NULL;
    static size_t buf_size = 0;
    
    size_t needed = strlen((const char*)val) + 1;
    if (needed > buf_size) {
        buf_size = needed * 2;
        buf = realloc(buf, buf_size);
    }
//    fprintf(stderr, "needed: %d\t buf_size: %d\n", needed, buf_size);
    
    strcpy(buf, (const char*)val);
    xmlFree(val);
    return buf;
}

// Initialize tokenizer
static void tokenizer_init(const char *path_str) {
    tokenizer.str = path_str;
    tokenizer.pos = 0;
}

// Skip whitespace and commas
static void skip_whitespace() {
    while (tokenizer.pos < strlen(tokenizer.str)) {
        char c = tokenizer.str[tokenizer.pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',') {
            tokenizer.pos++;
        } else {
            break;
        }
    }
}

// Get next number from path string
static float next_number() {
    skip_whitespace();
    
    char *endptr;
    float val = strtof(tokenizer.str + tokenizer.pos, &endptr);
    tokenizer.pos += (endptr - (tokenizer.str + tokenizer.pos));
    
    return val;
}

// Get next command character
static char next_command() {
    skip_whitespace();
    
    if (tokenizer.pos >= strlen(tokenizer.str)) return '\0';
    
    char c = tokenizer.str[tokenizer.pos];
    if (isalpha(c)) {
        tokenizer.pos++;
        return c;
    }
    return '\0';
}

// Check if more data available
static int has_more() {
    skip_whitespace();
    return tokenizer.pos < strlen(tokenizer.str);
}

// Plot a straight line segment
void plot_line(float x, float y, Matrix transform) {
    add_line(path_x, path_y, x, y, transform);
    path_x = x;
    path_y = y;
}

// Recursive helper for quadratic Bezier subdivision
static void subdivide_quadratic_bezier(float p0x, float p0y, float p1x, float p1y,
                                        float p2x, float p2y,
                                        Matrix transform, float max_error, int depth) {
    // Limit recursion depth
    if (depth > 16) {
        add_line(p0x, p0y, p2x, p2y, transform);
        return;
    }

    // Midpoints of control polygon
    float m01x = (p0x + p1x) * 0.5f;
    float m01y = (p0y + p1y) * 0.5f;
    float m12x = (p1x + p2x) * 0.5f;
    float m12y = (p1y + p2y) * 0.5f;

    // Point on curve at t=0.5
    float midx = (m01x + m12x) * 0.5f;
    float midy = (m01y + m12y) * 0.5f;

    // Estimate error: distance from curve midpoint to line segment midpoint
    float line_midx = (p0x + p2x) * 0.5f;
    float line_midy = (p0y + p2y) * 0.5f;
    float dx = midx - line_midx;
    float dy = midy - line_midy;
    float error = sqrtf(dx * dx + dy * dy);

    // If error is small enough, just draw a line
    if (error <= max_error) {
        add_line(p0x, p0y, p2x, p2y, transform);
    } else {
        // Subdivide and recurse on both halves
        subdivide_quadratic_bezier(p0x, p0y, m01x, m01y, midx, midy,
                                   transform, max_error, depth + 1);
        subdivide_quadratic_bezier(midx, midy, m12x, m12y, p2x, p2y,
                                   transform, max_error, depth + 1);
    }
}

// Approximate quadratic Bezier curve with line segments
// P0 = (path_x, path_y), P1 = control point, P2 = end point
// max_error: maximum deviation from true curve
void plot_quadratic_bezier(float cx, float cy, float x, float y, Matrix transform, float max_error) {
    subdivide_quadratic_bezier(path_x, path_y, cx, cy, x, y, transform, max_error, 0);
    path_x = x;
    path_y = y;
}

// Recursive helper for cubic Bezier subdivision
// Splits curve if error is too large, otherwise adds line segment
static void subdivide_cubic_bezier(float p0x, float p0y, float p1x, float p1y,
                                    float p2x, float p2y, float p3x, float p3y,
                                    Matrix transform, float max_error, int depth) {
    // Limit recursion depth to prevent infinite loops
    if (depth > 24) {
        add_line(p0x, p0y, p3x, p3y, transform);
        return;
    }
    
    // Midpoints of control polygon
    float m01x = (p0x + p1x) * 0.5f;
    float m01y = (p0y + p1y) * 0.5f;
    float m12x = (p1x + p2x) * 0.5f;
    float m12y = (p1y + p2y) * 0.5f;
    float m23x = (p2x + p3x) * 0.5f;
    float m23y = (p2y + p3y) * 0.5f;
    
    // Second level midpoints
    float m012x = (m01x + m12x) * 0.5f;
    float m012y = (m01y + m12y) * 0.5f;
    float m123x = (m12x + m23x) * 0.5f;
    float m123y = (m12y + m23y) * 0.5f;
    
    // Final midpoint (point on curve at t=0.5)
    float midx = (m012x + m123x) * 0.5f;
    float midy = (m012y + m123y) * 0.5f;
    
    // Estimate error: distance from curve midpoint to line segment midpoint
    float line_midx = (p0x + p3x) * 0.5f;
    float line_midy = (p0y + p3y) * 0.5f;
    float dx = midx - line_midx;
    float dy = midy - line_midy;
    float error = sqrtf(dx * dx + dy * dy);
    
    // If error is small enough, just draw a line
    if (error <= max_error) {
        add_line(p0x, p0y, p3x, p3y, transform);
    } else {
        // Subdivide and recurse on both halves
        subdivide_cubic_bezier(p0x, p0y, m01x, m01y, m012x, m012y, midx, midy,
                              transform, max_error, depth + 1);
        subdivide_cubic_bezier(midx, midy, m123x, m123y, m23x, m23y, p3x, p3y,
                              transform, max_error, depth + 1);
    }
}

// Approximate cubic Bezier curve with line segments
// P0 = (path_x, path_y), P1 = control1, P2 = control2, P3 = end point
// max_error: maximum deviation from true curve
void plot_cubic_bezier(float c1x, float c1y, float c2x, float c2y, float x, float y, Matrix transform, float max_error) {
    subdivide_cubic_bezier(path_x, path_y, c1x, c1y, c2x, c2y, x, y, transform, max_error, 0);
    path_x = x;
    path_y = y;
}

// Approximate arc with line segments
// rx, ry: radii
// x_axis_rotation: rotation in degrees
// large_arc_flag, sweep_flag: arc flags
// x, y: end point
void plot_arc(float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y, Matrix transform, float max_error) {
    // Handle degenerate cases
    if (path_x == x && path_y == y) return;
    if (rx == 0 || ry == 0) {
        plot_line(x, y, transform);
        return;
    }
    
    float rad = x_axis_rotation * M_PI / 180.0f;
    float cos_rot = cosf(rad);
    float sin_rot = sinf(rad);
    
    // Ensure radii are positive and large enough
    rx = fabsf(rx);
    ry = fabsf(ry);
    
    // Step 1: Compute center point
    // Transform to unit circle space
    float dx = (path_x - x) * 0.5f;
    float dy = (path_y - y) * 0.5f;
    float x1 = cos_rot * dx + sin_rot * dy;
    float y1 = -sin_rot * dx + cos_rot * dy;
    
    // Correct radii if too small
    float lambda = (x1 * x1) / (rx * rx) + (y1 * y1) / (ry * ry);
    if (lambda > 1.0f) {
        rx *= sqrtf(lambda);
        ry *= sqrtf(lambda);
    }
    
    // Compute center in unit circle space
    float sq = fmaxf(0.0f, (rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1) /
                            (rx * rx * y1 * y1 + ry * ry * x1 * x1));
    float coeff = sqrtf(sq);
    if (large_arc_flag == sweep_flag) coeff = -coeff;
    
    float cx1 = coeff * rx * y1 / ry;
    float cy1 = -coeff * ry * x1 / rx;
    
    // Transform back to original space
    float cx = cos_rot * cx1 - sin_rot * cy1 + (path_x + x) * 0.5f;
    float cy = sin_rot * cx1 + cos_rot * cy1 + (path_y + y) * 0.5f;
    
    // Step 2: Compute start and end angles
    float ux = (x1 - cx1) / rx;
    float uy = (y1 - cy1) / ry;
    float vx = (-x1 - cx1) / rx;
    float vy = (-y1 - cy1) / ry;
    
    float n = sqrtf(ux * ux + uy * uy);
    float p = ux;
    float theta1 = acosf(p / n);
    if (uy < 0) theta1 = -theta1;
    
    n = sqrtf((ux * ux + uy * uy) * (vx * vx + vy * vy));
    p = ux * vx + uy * vy;
    float dtheta = acosf(fmaxf(-1.0f, fminf(1.0f, p / n)));
    if (ux * vy - uy * vx < 0) dtheta = -dtheta;
    
    if (sweep_flag && dtheta < 0) {
        dtheta += 2.0f * M_PI;
    } else if (!sweep_flag && dtheta > 0) {
        dtheta -= 2.0f * M_PI;
    }
    
    // Step 3: Approximate arc with cubic Bezier curves
    // Split into segments of at most 90 degrees
    int segments = (int)ceilf(fabsf(dtheta) / (M_PI * 0.5f));
    float delta = dtheta / segments;
    float alpha = sinf(delta) * (sqrtf(4.0f + 3.0f * tanf(delta * 0.5f) * tanf(delta * 0.5f)) - 1.0f) / 3.0f;
    
    float cos_rot_rx = cos_rot * rx;
    float sin_rot_rx = sin_rot * rx;
    float cos_rot_ry = cos_rot * ry;
    float sin_rot_ry = sin_rot * ry;
    
    float theta = theta1;
    float cos_theta = cosf(theta);
    float sin_theta = sinf(theta);
    
    for (int i = 0; i < segments; i++) {
        float theta_next = theta + delta;
        float cos_theta_next = cosf(theta_next);
        float sin_theta_next = sinf(theta_next);
        
        // First control point
        float q1x = cos_theta - sin_theta * alpha;
        float q1y = sin_theta + cos_theta * alpha;
        float cp1x = cx + cos_rot_rx * q1x - sin_rot_ry * q1y;
        float cp1y = cy + sin_rot_rx * q1x + cos_rot_ry * q1y;
        
        // Second control point
        float q2x = cos_theta_next + sin_theta_next * alpha;
        float q2y = sin_theta_next - cos_theta_next * alpha;
        float cp2x = cx + cos_rot_rx * q2x - sin_rot_ry * q2y;
        float cp2y = cy + sin_rot_rx * q2x + cos_rot_ry * q2y;
        
        // End point
        float epx = cx + cos_rot_rx * cos_theta_next - sin_rot_ry * sin_theta_next;
        float epy = cy + sin_rot_rx * cos_theta_next + cos_rot_ry * sin_theta_next;
        
        plot_cubic_bezier(cp1x, cp1y, cp2x, cp2y, epx, epy, transform, max_error);
        
        theta = theta_next;
        cos_theta = cos_theta_next;
        sin_theta = sin_theta_next;
    }
}

void set_pos(float x, float y) {
    path_x = x;
    path_y = y;
}

// Process path data
void parse_path_data(const char *path_str, Matrix transform, float max_error) {
    tokenizer_init(path_str);

    // Previous control points for smooth curves
    static float prev_cp_x = 0.0f;
    static float prev_cp_y = 0.0f;

    // Current position in path
    path_x = 0.0f;
    path_y = 0.0f;
    
    char cmd = '\0';
    float subpath_start_x = 0, subpath_start_y = 0;
    
    while (has_more()) {
        // Get command (or repeat last if number follows)
        char c = next_command();
        if (c) {
            cmd = c;
        } else if (!isdigit(tokenizer.str[tokenizer.pos]) && 
                   tokenizer.str[tokenizer.pos] != '.' &&
                   tokenizer.str[tokenizer.pos] != '-' &&
                   tokenizer.str[tokenizer.pos] != '+') {
            break;
        }
        // else: implicit command repetition
        
        // Convert to absolute coordinates
        int absolute = isupper(cmd);
        char cmd_lower = tolower(cmd);
        
        float x, y, x1, y1, x2, y2, rx, ry, rotation;
        int large_arc, sweep;
        
        switch (cmd_lower) {
            case 'm': // Move
                x = next_number();
                y = next_number();
                if (!absolute) { x += path_x; y += path_y; }
                path_x = x;
                path_y = y;
                subpath_start_x = x;
                subpath_start_y = y;
                // After moveto, implicit lineto for remaining coordinates
                cmd = absolute ? 'L' : 'l';
                prev_cp_x = path_x;
                prev_cp_y = path_y;
                break;
                
            case 'l': // Line
                x = next_number();
                y = next_number();
                if (!absolute) { x += path_x; y += path_y; }
                plot_line(x, y, transform);
                prev_cp_x = path_x;
                prev_cp_y = path_y;
                break;
                
            case 'h': // Horizontal line
                x = next_number();
                if (!absolute) x += path_x;
                plot_line(x, path_y, transform);
                prev_cp_x = path_x;
                prev_cp_y = path_y;
                break;
                
            case 'v': // Vertical line
                y = next_number();
                if (!absolute) y += path_y;
                plot_line(path_x, y, transform);
                prev_cp_x = path_x;
                prev_cp_y = path_y;
                break;
                
            case 'c': // Cubic Bezier
                x1 = next_number(); y1 = next_number();
                x2 = next_number(); y2 = next_number();
                x = next_number();  y = next_number();
                if (!absolute) {
                    x1 += path_x; y1 += path_y;
                    x2 += path_x; y2 += path_y;
                    x += path_x;  y += path_y;
                }
                plot_cubic_bezier(x1, y1, x2, y2, x, y, transform, max_error);
                prev_cp_x = x2;
                prev_cp_y = y2;
                break;
                
            case 's': // Smooth cubic Bezier
                x2 = next_number(); y2 = next_number();
                x = next_number();  y = next_number();
                if (!absolute) {
                    x2 += path_x; y2 += path_y;
                    x += path_x;  y += path_y;
                }
                // Reflect previous control point across current point
                x1 = 2.0f * path_x - prev_cp_x;
                y1 = 2.0f * path_y - prev_cp_y;
                plot_cubic_bezier(x1, y1, x2, y2, x, y, transform, max_error);
                prev_cp_x = x2;
                prev_cp_y = y2;
                break;
                
            case 'q': // Quadratic Bezier
                x1 = next_number(); y1 = next_number();
                x = next_number();  y = next_number();
                if (!absolute) {
                    x1 += path_x; y1 += path_y;
                    x += path_x;  y += path_y;
                }
                plot_quadratic_bezier(x1, y1, x, y, transform, max_error);
                prev_cp_x = x1;
                prev_cp_y = y1;
                break;
                
            case 't': // Smooth quadratic Bezier
                x = next_number();  y = next_number();
                if (!absolute) {
                    x += path_x;  y += path_y;
                }
                // Reflect previous control point across current point
                x1 = 2.0f * path_x - prev_cp_x;
                y1 = 2.0f * path_y - prev_cp_y;
                plot_quadratic_bezier(x1, y1, x, y, transform, max_error);
                prev_cp_x = x1;
                prev_cp_y = y1;
                break;
                
            case 'a': // Arc
                rx = next_number();
                ry = next_number();
                rotation = next_number();
                large_arc = (int)next_number();
                sweep = (int)next_number();
                x = next_number();
                y = next_number();
                if (!absolute) { x += path_x; y += path_y; }
                plot_arc(rx, ry, rotation, large_arc, sweep, x, y, transform, max_error);
                prev_cp_x = path_x;
                prev_cp_y = path_y;
                break;
                
            case 'z': // Close path
                plot_line(subpath_start_x, subpath_start_y, transform);
                break;
                
            default:
                break;
        }
    }
}

