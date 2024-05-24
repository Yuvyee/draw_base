#ifndef META_GRAPH_H
#define META_GRAPH_H

#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define PER_BP_NUM 10

#define COLOR_ELE_NUM 4

#define GRAPH_LIST_HEAD -1

#define BASIC_PRIMITIVE_NONE 0
#define BASIC_PRIMITIVE_POINT 1
#define BASIC_PRIMITIVE_LINE 2
#define BASIC_PRIMITIVE_RECT 3
#define BASIC_PRIMITIVE_POLYGON 4
#define BASIC_PRIMITIVE_GRAPH 5
#define BASIC_PRIMITIVE_CIRCLE 6
#define BASIC_PRIMITIVE_ELLIPSE 7
namespace MetaGraph
{
typedef struct meta_label
{
    int labels_num;
    std::vector<std::string> labels_ele;
} meta_label_t;

typedef struct shared_attr
{
    float value;
    float color_f[COLOR_ELE_NUM];   // rgba
    uint8_t color_u[COLOR_ELE_NUM]; // bgra
    shared_attr() : value(-1), color_u{255, 0, 0, 1}
    {
    }

    int set_attr(float value_, uint8_t *color_)
    {
        if (value_ < 0)
            return -1;

        value = value;
        memcpy(color_u, color_, sizeof(shared_attr::color_u));
        return 0;
    };

    void color_format_u2f()
    {
        color_f[0] = 1 / color_u[2];
        color_f[1] = 1 / color_u[1];
        color_f[2] = 1 / color_u[0];
        color_f[3] = 1 / color_u[3];
    }

} shared_attr_t;

typedef struct meta_attr
{
    void *next_prim_node;
    struct main_node *main_node;
    struct shared_attr *shared_attr;
    uint32_t label_index;
    float score;
} meta_attr_t;

typedef struct extend_primitive_attr
{
    uint8_t filled_color[COLOR_ELE_NUM];
} extend_primitive_attr_t;

typedef struct basic_polygon_attr
{
    int vertex_num;
    bool closed;
    bool is_directed;
    uint8_t filled_color[COLOR_ELE_NUM];
} basic_polygon_attr_t;

typedef struct basic_graph_attr
{
    bool is_directed;
    int vertex_num;
    std::vector<std::pair<uint16_t, uint16_t>> connection_set;
} basic_graph_attr_t;

typedef struct meta_point
{
    std::pair<uint16_t, uint16_t> loc;
    meta_point() : loc(-1, -1)
    {
    }
    meta_point(uint16_t x, uint16_t y) : loc(x, y)
    {
    }
    void set_loc(uint16_t x, uint16_t y)
    {
        loc = std::pair(x, y);
    }
    ~meta_point()
    {
    }
} meta_point_t;

typedef struct meta_line
{
    meta_point_t line_begin;
    meta_point_t line_end;
    std::vector<meta_point_t> control_points;
    meta_line()
    {
    }

    void set_endpoint(const meta_point_t &begin, const meta_point_t &end)
    {
        line_begin = begin;
        line_end = end;
    }
    void set_control_points(const meta_point_t &control_point)
    {
        control_points.push_back(control_point);
    }
    ~meta_line()
    {
    }
} meta_line_t;

typedef struct basic_point
{
    meta_attr_t local_attr;
    meta_point_t point_info;
    basic_point()
    {
    }

    basic_point(uint16_t x, uint16_t y) : point_info(x, y)
    {
    }

    ~basic_point()
    {
    }
} basic_point_t;

typedef struct basic_line
{
    meta_attr_t local_attr;
    meta_line_t line_info;
    basic_line()
    {
    }
    ~basic_line()
    {
    }
} basic_line_t;

typedef struct basic_rect
{
    meta_attr_t local_attr;
    meta_point_t upper_left;
    meta_point_t lower_right;
    basic_rect()
    {
    }
    void set_diagonal_point(meta_point_t ul, meta_point_t lr)
    {
        upper_left = ul;
        lower_right = lr;
    }
    ~basic_rect()
    {
    }
} basic_rect_t;

typedef struct basic_vertex_set
{
    meta_attr_t local_attr;
    std::vector<meta_point_t> vertex_set;
    basic_vertex_set()
    {
    }
    void set_vertex(meta_point_t vertex)
    {
        vertex_set.push_back(vertex);
    }
    ~basic_vertex_set()
    {
    }
} basic_vertex_set_t;

typedef struct basic_circle
{
    uint16_t radius;
    meta_attr_t local_attr;
    meta_point_t center;
} basic_circle_t;

typedef struct basic_ellipse
{
    uint16_t x_radius;
    uint16_t y_radius;
    float rotate_theta;
    meta_point_t center;
    meta_attr_t local_attr;
} basic_ellipse_t;

typedef struct main_node
{
    int primitive_type;
    int primitive_num;
    void *graph_primitive_head;
    void *graph_primitive_tail;
    void *graph_primitive_attr;
    struct shared_attr *local_attr;
    struct main_node *main_node;
} main_node_t;

typedef struct Graph_List
{
    int main_point_num;
    main_node_t *main_node;
    shared_attr_t *glob_attr;
    meta_label_t graph_label_info;
} Graph_List_t;

typedef void *b_primitive_handle_t;

int init_global_tasks_map(std::unordered_map<std::string, Graph_List_t *> &global_tasks_map,
                          const std::vector<std::string> &tasks_list);

int update_global_tasks_map(const std::string &config_cotent, std::unordered_map<std::string, Graph_List_t *> &global_tasks_map,
                            const std::string &task_name);

int construct_basic_primitives(uint8_t *metadata, std::unordered_map<std::string, Graph_List_t *> global_tasks_map);

int print_global_tasks_map(const std::unordered_map<std::string, Graph_List_t *> &global_tasks_map);

int uninit_graph_list_spec(std::unordered_map<std::string, Graph_List_t *> global_tasks_map, const std::string &task_name);

int uninit_global_tasks_map(std::unordered_map<std::string, Graph_List_t *> global_tasks_map);

// std::unordered_map<int, std::unordered_map<std::string, Graph_List_t *>> multiDevices_multiTasks_map;

} // namespace MetaGraph
#endif // META_GRAPH_H
