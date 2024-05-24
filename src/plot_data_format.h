#ifndef PLOT_DATA_FORMAT_H
#define PLOT_DATA_FORMAT_H

#include "opencv2/opencv.hpp"
#include "yaml-cpp/yaml.h"

#define TASK_CONFIG_FILE __MODEL_TASKS_CONFIG_PATH__

typedef enum COMP_TYPE
{
    POINTS_2D = 1,
    CIRCLE = 2,
    ELLIPSE = 3,
} COMP_TYPE_T;

typedef struct weight_comp
{
    union {
        uint32_t label_index;
        int weight_value;
    } weight;
    float score;
} weight_comp_t;

typedef struct point_2d
{
    uint16_t loc[2];
} point_2d_t;

typedef struct points_2d_comp
{
    int points_num;
    struct points_2d_comp *next_pcomp;
    struct weight_comp weight;
    struct point_2d point_set[0];
} points_2d_comp_t;

typedef struct circle_comp
{
    struct circle_comp *next_ccomp;
    struct weight_comp weight;
    struct point_2d centre;
    int radius;
} circle_comp_t;

typedef struct ellipse_comp
{
    struct ellipse_comp *next_ecomp;
    struct weight_comp weight;
    struct point_2d centre;
    int x_radius;
    int y_radius;
    float rotate_theta;
} ellipse_comp_t;

typedef struct transit_node
{
    int comp_num;
    int avail_space;
    COMP_TYPE_T comp_type;
    void *comp_node_head;
    void *comp_node_tail;
    struct transit_node *next_tnode;
} transit_node_t;

typedef struct main_transit_node
{
    int transit_node_num;
    int avail_transit_node_num;
    struct transit_node *tnode_list[0];
} main_transit_node_t;


#endif // !PLOT_DATA_FORMAT_H