#include "metagraph.h"
#include "opencv2/opencv.hpp"

#define YAML_FILE_HEADER "%YAML:1.0\n"

namespace MetaGraph
{
// extern std::unordered_map<int, std::unordered_map<std::string, Graph_List_t *>> multiDevices_multiTasks_map;

static std::vector<std::string> global_tasks_list;

static Graph_List_t *construct_graph_list(const std::string &task_node_content, const std::string &task_name);

static int construct_main_point_list(const cv::FileNode &basic_primitives_content, Graph_List_t *gl_head);

Graph_List_t *construct_graph_list(const std::string &task_node_content, const std::string &task_name)
{

    cv::FileStorage task_node(std::string(YAML_FILE_HEADER) + task_node_content,
                              cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML | cv::FileStorage::MEMORY);
    if (!task_node.isOpened())
    {
        printf("[ERROR]: task_node %s open failed!\n", task_name.c_str());
        return NULL;
    }
    Graph_List_t *gl_head = new Graph_List_t;
    cv::FileNode labels_list_node = task_node["labels_list"];
    cv::FileNode basic_primitives_node = task_node["basic_primitives"];

    gl_head->main_point_num = 0;
    gl_head->main_node = nullptr;
    if (labels_list_node.empty())
    {
        gl_head->graph_label_info.labels_num = 0;
    }
    else
    {
        for (auto it = labels_list_node.begin(); it != labels_list_node.end(); ++it)
            gl_head->graph_label_info.labels_ele.push_back((std::string)(*it));
        gl_head->graph_label_info.labels_num = gl_head->graph_label_info.labels_ele.size();
    }
    if (basic_primitives_node.empty())
    {
        printf("[ERROR]: basic_primitives_node of %s is empty!\n", task_name.c_str());
        goto construct_fail;
    }
    {

        if (construct_main_point_list(basic_primitives_node, gl_head))
            goto construct_fail;
    }

    if (gl_head->main_point_num == 0)
        goto construct_fail;

    return gl_head;

construct_fail:
    delete gl_head;
    return nullptr;
}

static int construct_main_point_list(const cv::FileNode &basic_primitives_node, Graph_List_t *gl_head)
{
    if (basic_primitives_node.empty())
    {
        printf("[ERROR]: basic_primitives_node is empty!\n");
        return -1;
    }

    cv::FileNode glob_attr = basic_primitives_node["glob_attr"];
    gl_head->glob_attr = new shared_attr_t;
    if (!glob_attr.empty())
    {
        gl_head->glob_attr->value = (float)glob_attr["value"];
        int i = 0;
        for (cv::FileNodeIterator it = glob_attr["color"].begin(); it != glob_attr["color"].end() && i < COLOR_ELE_NUM;
             ++it, ++i)
            gl_head->glob_attr->color_u[i] = (int)(*it);
    }
    main_node_t **cur_mp_p = &gl_head->main_node;
    for (int i = 1;; i++)
    {
        std::string bp_name = std::string("bp") + std::to_string(i);
        cv::FileNode bp_node = basic_primitives_node[bp_name];
        if (bp_node.empty())
        {

            break;
        }
        int comp_type = (int)bp_node["comp_type"];
        main_node_t *cur_mp = new main_node_t;
        cur_mp->primitive_type = comp_type;
        cur_mp->primitive_num = 1;
        cv::FileNode local_attr = bp_node["local_attr"];
        int j = 0;
        if (local_attr.empty())
            cur_mp->local_attr = gl_head->glob_attr;
        else
        {
            cur_mp->local_attr = new shared_attr_t;
            cur_mp->local_attr->value = (float)local_attr["value"];
            for (cv::FileNodeIterator it = local_attr["color"].begin();
                 it != local_attr["color"].end() && j < COLOR_ELE_NUM; ++it, ++j)
            {
                cur_mp->local_attr->color_u[j] = (int)(*it);
            }
        }
        switch (comp_type)
        {
        case BASIC_PRIMITIVE_NONE: {
            struct meta_attr *bp_comp = new meta_attr;
            bp_comp->main_node = cur_mp;
            bp_comp->next_prim_node = nullptr;
            bp_comp->shared_attr = cur_mp->local_attr;
            cur_mp->graph_primitive_head = bp_comp;
            cur_mp->graph_primitive_tail = bp_comp;
            cur_mp->graph_primitive_attr = nullptr;
        }
        break;
        case BASIC_PRIMITIVE_POINT: {
            struct basic_point *bp_comp = new basic_point;
            bp_comp->local_attr.main_node = cur_mp;
            bp_comp->local_attr.next_prim_node = nullptr;
            bp_comp->local_attr.shared_attr = cur_mp->local_attr;
            cur_mp->graph_primitive_head = bp_comp;
            cur_mp->graph_primitive_tail = bp_comp;
            cur_mp->graph_primitive_attr = nullptr;
        }
        break;
        case BASIC_PRIMITIVE_LINE: {
            struct basic_line *bp_comp = new basic_line;
            bp_comp->local_attr.main_node = cur_mp;
            bp_comp->local_attr.next_prim_node = nullptr;
            bp_comp->local_attr.shared_attr = cur_mp->local_attr;
            cur_mp->graph_primitive_head = bp_comp;
            cur_mp->graph_primitive_tail = bp_comp;
            cur_mp->graph_primitive_attr = nullptr;
        }
        break;
        case BASIC_PRIMITIVE_RECT: {
            struct basic_rect *bp_comp = new basic_rect;
            struct extend_primitive_attr *exp_attr = new extend_primitive_attr;
            int k = 0;
            for (cv::FileNodeIterator it = local_attr["filled_color"].begin();
                 it != local_attr["filled_color"].end() && k < COLOR_ELE_NUM; ++it, ++k)
                exp_attr->filled_color[k] = (int)(*it);
            bp_comp->local_attr.main_node = cur_mp;
            bp_comp->local_attr.next_prim_node = nullptr;
            bp_comp->local_attr.shared_attr = cur_mp->local_attr;
            cur_mp->graph_primitive_head = bp_comp;
            cur_mp->graph_primitive_tail = bp_comp;
            cur_mp->graph_primitive_attr = exp_attr;
        }
        break;
        case BASIC_PRIMITIVE_POLYGON: {
            struct basic_vertex_set *bp_comp = new basic_vertex_set;
            struct basic_polygon_attr *ploygon_attr = new basic_polygon_attr;
            int k = 0;
            ploygon_attr->vertex_num = (int)bp_node["Edges_num"] + 1;
            ploygon_attr->closed = (int(bp_node["Closed"]) != 0);
            ploygon_attr->is_directed = (int(bp_node["Directed"]) != 0);
            if (ploygon_attr->closed)
            {
                for (cv::FileNodeIterator it = local_attr["filled_color"].begin();
                     it != local_attr["filled_color"].end() && k < COLOR_ELE_NUM; ++it, ++k)
                    ploygon_attr->filled_color[k] = (int)(*it);
            }
            else
                memset(ploygon_attr->filled_color, 0, sizeof(basic_polygon_attr_t::filled_color));

            bp_comp->local_attr.main_node = cur_mp;
            bp_comp->local_attr.next_prim_node = nullptr;
            bp_comp->local_attr.shared_attr = cur_mp->local_attr;
            cur_mp->graph_primitive_head = bp_comp;
            cur_mp->graph_primitive_tail = bp_comp;
            cur_mp->graph_primitive_attr = ploygon_attr;
        }
        break;
        case BASIC_PRIMITIVE_GRAPH: {
            struct basic_vertex_set *bp_comp = new basic_vertex_set;
            struct basic_graph_attr *graph_attr = new basic_graph_attr;
            graph_attr->vertex_num = (int)bp_node["Points_num"];
            graph_attr->is_directed = (int(bp_node["Directed"]) != 0);
            for (cv::FileNodeIterator it = bp_node["Connect_seq"].begin(); it != bp_node["Connect_seq"].end(); ++it)
            {
                uint16_t first = (int)(*it)[0];
                uint16_t second = (int)(*it)[1];
                graph_attr->connection_set.emplace_back(first, second);
            }
            bp_comp->local_attr.main_node = cur_mp;
            bp_comp->local_attr.next_prim_node = nullptr;
            bp_comp->local_attr.shared_attr = cur_mp->local_attr;
            cur_mp->graph_primitive_head = bp_comp;
            cur_mp->graph_primitive_tail = bp_comp;
            cur_mp->graph_primitive_attr = graph_attr;
        }
        break;
        case BASIC_PRIMITIVE_CIRCLE: {
            struct basic_circle *bp_comp = new basic_circle;
            struct extend_primitive_attr *exp_attr = new extend_primitive_attr;
            int k = 0;
            for (cv::FileNodeIterator it = local_attr["filled_color"].begin();
                 it != local_attr["filled_color"].end() && k < COLOR_ELE_NUM; ++it, ++k)
                exp_attr->filled_color[k] = (int)(*it);
            bp_comp->local_attr.main_node = cur_mp;
            bp_comp->local_attr.next_prim_node = nullptr;
            bp_comp->local_attr.shared_attr = cur_mp->local_attr;
            cur_mp->graph_primitive_head = bp_comp;
            cur_mp->graph_primitive_tail = bp_comp;
            cur_mp->graph_primitive_attr = exp_attr;
        }
        break;
        case BASIC_PRIMITIVE_ELLIPSE: {
            struct basic_ellipse *bp_comp = new basic_ellipse;
            struct extend_primitive_attr *exp_attr = new extend_primitive_attr;
            int k = 0;
            for (cv::FileNodeIterator it = local_attr["filled_color"].begin();
                 it != local_attr["filled_color"].end() && k < COLOR_ELE_NUM; ++it, ++k)
                exp_attr->filled_color[k] = (int)(*it);

            bp_comp->local_attr.main_node = cur_mp;
            bp_comp->local_attr.next_prim_node = nullptr;
            bp_comp->local_attr.shared_attr = cur_mp->local_attr;
            cur_mp->graph_primitive_head = bp_comp;
            cur_mp->graph_primitive_tail = bp_comp;
            cur_mp->graph_primitive_attr = exp_attr;
        }
        break;
        default:
            printf("[Error]: Wrong comp_type of %s, pass it!", bp_name.c_str());
            continue;
        }
        gl_head->main_point_num++;
        *cur_mp_p = cur_mp;
        cur_mp_p = &cur_mp->main_node;
    }
    return 0;
}

static int destory_primitives_node(main_node_t *main_node)
{
    int ret = 0;
    if (main_node == nullptr)
    {
        printf("[INFO]: free resource successfully!\n");
        return ret;
    }

    switch (main_node->primitive_type)
    {
    case BASIC_PRIMITIVE_NONE: {
        meta_attr_t *m_attr = static_cast<meta_attr_t *>(main_node->graph_primitive_head);
        delete m_attr;
    }
    break;
    case BASIC_PRIMITIVE_POINT: {
        basic_point_t *b_point = static_cast<basic_point_t *>(main_node->graph_primitive_head);
        while (b_point)
        {
            basic_point_t *b_point_temp = b_point;
            b_point = static_cast<basic_point_t *>(b_point->local_attr.next_prim_node);
            delete b_point_temp;
        }
    }
    break;
    case BASIC_PRIMITIVE_LINE: {
        basic_line_t *b_line = static_cast<basic_line_t *>(main_node->graph_primitive_head);
        while (b_line)
        {
            basic_line_t *b_line_temp = b_line;
            b_line = static_cast<basic_line_t *>(b_line->local_attr.next_prim_node);
            delete b_line_temp;
        }
    }
    break;
    case BASIC_PRIMITIVE_RECT: {
        basic_rect_t *b_rect = static_cast<basic_rect_t *>(main_node->graph_primitive_head);
        struct extend_primitive_attr *exp_attr = static_cast<extend_primitive_attr *>(main_node->graph_primitive_attr);
        if (exp_attr)
            delete exp_attr;
        while (b_rect)
        {
            basic_rect_t *b_rect_temp = b_rect;
            b_rect = static_cast<basic_rect_t *>(b_rect->local_attr.next_prim_node);
            delete b_rect_temp;
        }
    }
    break;
    case BASIC_PRIMITIVE_POLYGON:
    case BASIC_PRIMITIVE_GRAPH: {
        basic_vertex_set_t *b_vertex_set = static_cast<basic_vertex_set_t *>(main_node->graph_primitive_head);
        if (main_node->primitive_type == BASIC_PRIMITIVE_POLYGON)
        {
            basic_polygon_attr_t *bp_attr = static_cast<basic_polygon_attr_t *>(main_node->graph_primitive_attr);
            if (bp_attr)
                delete bp_attr;
        }
        else
        {
            basic_graph_attr_t *bg_attr = static_cast<basic_graph_attr_t *>(main_node->graph_primitive_attr);
            if (bg_attr)
                delete bg_attr;
        }
        while (b_vertex_set)
        {
            basic_vertex_set_t *b_vertex_set_temp = b_vertex_set;
            b_vertex_set = static_cast<basic_vertex_set_t *>(b_vertex_set->local_attr.next_prim_node);
            delete b_vertex_set_temp;
        }
    }
    break;
    case BASIC_PRIMITIVE_CIRCLE: {
        basic_circle_t *b_circle = static_cast<basic_circle_t *>(main_node->graph_primitive_head);
        struct extend_primitive_attr *exp_attr = static_cast<extend_primitive_attr *>(main_node->graph_primitive_attr);
        if (exp_attr)
            delete exp_attr;
        while (b_circle)
        {
            basic_circle_t *b_circle_temp = b_circle;
            b_circle = static_cast<basic_circle_t *>(b_circle->local_attr.next_prim_node);
            delete b_circle_temp;
        }
    }
    break;
    case BASIC_PRIMITIVE_ELLIPSE: {
        basic_ellipse_t *b_ellipse = static_cast<basic_ellipse_t *>(main_node->graph_primitive_head);
        struct extend_primitive_attr *exp_attr = static_cast<extend_primitive_attr *>(main_node->graph_primitive_attr);
        if (exp_attr)
            delete exp_attr;
        while (b_ellipse)
        {
            basic_ellipse_t *b_ellipse_temp = b_ellipse;
            b_ellipse = static_cast<basic_ellipse_t *>(b_ellipse->local_attr.next_prim_node);
            delete b_ellipse_temp;
        }
    }
    break;
    default:
        printf("[ERROR]: Failed to free the type %d of primitive!\n", main_node->primitive_type);
        ret = -1;
        break;
    }
    if (main_node->local_attr)
        delete main_node->local_attr;
    destory_primitives_node(main_node->main_node);
    delete main_node;

    return ret;
}

static int destory_graph_list(Graph_List_t *gl_head)
{
    int ret = 0;
    ret = destory_primitives_node(gl_head->main_node);
    if (gl_head->glob_attr)
        delete gl_head->glob_attr;

    delete gl_head;
    // global_tasks_map.erase(it);
    return ret;
}

int init_global_tasks_map(std::unordered_map<std::string, Graph_List_t *> &global_tasks_map,
                          const std::vector<std::string> &tasks_list)
{
    for (const std::string &task_name : tasks_list)
    {
        global_tasks_map[task_name] = nullptr;
        global_tasks_list.emplace_back(task_name);
    }
    return 0;
}

int update_global_tasks_map(const std::string &config_cotent,
                            std::unordered_map<std::string, Graph_List_t *> &global_tasks_map,
                            const std::string &task_name)
{
    Graph_List_t *gl_head = construct_graph_list(config_cotent, task_name);
    if (!gl_head)
    {
        printf("[ERROR]: failed to build %s graph list\n", task_name.c_str());
        return -1;
    }
    if (global_tasks_map[task_name])
        uninit_graph_list_spec(global_tasks_map, task_name);
    global_tasks_map[task_name] = gl_head;

    return 0;
}

int construct_basic_primitives(uint8_t *metadata, std::unordered_map<std::string, Graph_List_t *> global_tasks_map)
{
    uint8_t task_id = metadata[0];
    uint8_t mg_num = metadata[1];
    std::vector<uint8_t> mgs_num_vec;
    // auto it = std::next(global_tasks_map.begin(), task_id);
    std::string task_name = global_tasks_list[task_id];
    auto it = global_tasks_map.find(task_name);
    if (it == global_tasks_map.end())
    {
        printf("[ERROR]: Get wrong task id from metadata!\n");
        return -1;
    }
    main_node_t *main_node = it->second->main_node;
    uint8_t *cur_md = metadata + mg_num + 2;

    for (uint8_t i = 0; i < mg_num; i++)
        mgs_num_vec.push_back(metadata[i + 2]);

    for (const auto &mgs_num : mgs_num_vec)
    {
        switch (main_node->primitive_type)
        {
        case BASIC_PRIMITIVE_NONE: {
            if (mgs_num != 1)
                printf("[WARNING]: None type num must be 1 or 0!\n");

            meta_attr_t *m_attr = static_cast<meta_attr_t *>(main_node->graph_primitive_head);
            memcpy(&m_attr->label_index, cur_md, sizeof(meta_attr::label_index));
            memcpy(&m_attr->score, cur_md + sizeof(meta_attr::label_index), sizeof(meta_attr::score));
            main_node->primitive_num = 1;
            cur_md = cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score);
        }
        break;
        case BASIC_PRIMITIVE_POINT: {
            int avail_space = main_node->primitive_num;
            int rest_data = mgs_num;
            basic_point_t *b_point = static_cast<basic_point_t *>(main_node->graph_primitive_head);
            while (avail_space > 0 && b_point != nullptr)
            {
                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));
                memcpy(&b_point->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_point->local_attr.score, cur_md + sizeof(meta_attr::label_index), sizeof(meta_attr::score));
                uint16_t xy[2] = {0};
                memcpy(&xy, temp_data_p, sizeof(xy));
                b_point->point_info.loc = std::make_pair(xy[0], xy[1]);
                cur_md = cur_md + sizeof(meta_point_t::loc) + sizeof(meta_attr::label_index) + sizeof(meta_attr::score);
                b_point = static_cast<basic_point_t *>(b_point->local_attr.next_prim_node);
                avail_space--;
                rest_data--;
            }
            b_point = static_cast<basic_point_t *>(main_node->graph_primitive_tail);
            while (rest_data > 0)
            {
                basic_point_t *b_point_temp = new basic_point_t;
                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));
                memcpy(&b_point_temp->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_point_temp->local_attr.score, cur_md + sizeof(meta_attr::label_index),
                       sizeof(meta_attr::score));
                uint16_t xy[2] = {0};
                memcpy(&xy, temp_data_p, sizeof(xy));
                b_point_temp->point_info.loc = std::make_pair(xy[0], xy[1]);
                b_point_temp->local_attr.next_prim_node = nullptr;
                b_point_temp->local_attr.main_node = main_node;
                b_point_temp->local_attr.shared_attr = main_node->local_attr;
                cur_md = cur_md + sizeof(meta_point_t::loc) + sizeof(meta_attr::label_index) + sizeof(meta_attr::score);
                b_point->local_attr.next_prim_node = b_point_temp;
                b_point = b_point_temp;
                rest_data--;
            }
            main_node->graph_primitive_tail = b_point;
            main_node->primitive_num = mgs_num;
        }
        break;
        case BASIC_PRIMITIVE_LINE: {
            int avail_space = main_node->primitive_num;
            int rest_data = mgs_num;
            basic_line_t *b_line = static_cast<basic_line_t *>(main_node->graph_primitive_head);
            while (avail_space > 0 && b_line != nullptr)
            {
                uint8_t control_points_num = cur_md[sizeof(meta_attr::label_index) + sizeof(meta_attr::score)];
                uint16_t *temp_data_p = reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) +
                                                                     sizeof(meta_attr::score) + 1);
                memcpy(&b_line->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_line->local_attr.score, cur_md + sizeof(meta_attr::label_index), sizeof(meta_attr::score));
                uint16_t points[4] = {0};
                memcpy(&points, temp_data_p, sizeof(points));
                b_line->line_info.set_endpoint(meta_point_t(points[0], points[1]), meta_point_t(points[2], points[3]));
                for (uint8_t count = control_points_num; count > 0; count--)
                {
                    temp_data_p = temp_data_p + sizeof(meta_point_t::loc) * 2 / sizeof(meta_point_t::loc.first);
                    uint16_t xy[2] = {0};
                    memcpy(&xy, temp_data_p, sizeof(xy));
                    b_line->line_info.set_control_points(meta_point_t(xy[0], xy[1]));
                }
                cur_md = cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) +
                         sizeof(meta_point_t::loc) * 2 + sizeof(meta_point_t) * control_points_num;

                b_line = static_cast<basic_line_t *>(b_line->local_attr.next_prim_node);
                avail_space--;
                rest_data--;
            }
            b_line = static_cast<basic_line_t *>(main_node->graph_primitive_tail);
            while (rest_data > 0)
            {
                basic_line_t *b_line_temp = new basic_line_t;
                uint8_t control_points_num = cur_md[sizeof(meta_attr::label_index) + sizeof(meta_attr::score)];
                uint16_t *temp_data_p = reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) +
                                                                     sizeof(meta_attr::score) + 1);
                memcpy(&b_line_temp->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_line_temp->local_attr.score, cur_md + sizeof(meta_attr::label_index),
                       sizeof(meta_attr::score));
                uint16_t points[4] = {0};
                memcpy(&points, temp_data_p, sizeof(points));
                b_line_temp->line_info.set_endpoint(meta_point_t(points[0], points[1]),
                                                    meta_point_t(points[2], points[3]));
                for (uint8_t count = control_points_num; count > 0; count--)
                {
                    temp_data_p = temp_data_p + sizeof(meta_point_t::loc) * 2 / sizeof(meta_point_t::loc.first);
                    uint16_t xy[2] = {0};
                    memcpy(&xy, temp_data_p, sizeof(xy));
                    b_line_temp->line_info.set_control_points(meta_point_t(xy[0], xy[1]));
                }
                b_line_temp->local_attr.next_prim_node = nullptr;
                b_line_temp->local_attr.main_node = main_node;
                b_line_temp->local_attr.shared_attr = main_node->local_attr;
                cur_md = cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) +
                         sizeof(meta_point_t::loc) * 2 + sizeof(meta_point_t) * control_points_num;

                b_line->local_attr.next_prim_node = b_line_temp;
                b_line = b_line_temp;
                rest_data--;
            }
            main_node->graph_primitive_tail = b_line;
            main_node->primitive_num = mgs_num;
        }
        break;
        case BASIC_PRIMITIVE_RECT: {
            int avail_space = main_node->primitive_num;
            int rest_data = mgs_num;
            basic_rect_t *b_rect = static_cast<basic_rect_t *>(main_node->graph_primitive_head);
            while (avail_space > 0 && b_rect != nullptr)
            {
                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));

                memcpy(&b_rect->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_rect->local_attr.score, cur_md + sizeof(meta_attr::label_index), sizeof(meta_attr::score));
                uint16_t points[4] = {0};
                memcpy(&points, temp_data_p, sizeof(points));
                b_rect->set_diagonal_point(meta_point_t(points[0], points[1]), meta_point_t(points[2], points[3]));

                cur_md =
                    cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) + sizeof(meta_point_t::loc) * 2;
                b_rect = static_cast<basic_rect_t *>(b_rect->local_attr.next_prim_node);
                avail_space--;
                rest_data--;
            }
            b_rect = static_cast<basic_rect_t *>(main_node->graph_primitive_tail);
            while (rest_data > 0)
            {
                basic_rect_t *b_rect_temp = new basic_rect_t;
                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));
                memcpy(&b_rect_temp->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_rect_temp->local_attr.score, cur_md + sizeof(meta_attr::label_index),
                       sizeof(meta_attr::score));
                uint16_t points[4] = {0};
                memcpy(&points, temp_data_p, sizeof(points));
                b_rect_temp->set_diagonal_point(meta_point_t(points[0], points[1]), meta_point_t(points[2], points[3]));
                b_rect_temp->local_attr.next_prim_node = nullptr;
                b_rect_temp->local_attr.main_node = main_node;
                b_rect_temp->local_attr.shared_attr = main_node->local_attr;
                cur_md =
                    cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) + sizeof(meta_point_t::loc) * 2;
                b_rect->local_attr.next_prim_node = b_rect_temp;
                b_rect = b_rect_temp;
                rest_data--;
            }
            main_node->graph_primitive_tail = b_rect;
            main_node->primitive_num = mgs_num;
        }
        break;
        case BASIC_PRIMITIVE_POLYGON:
        case BASIC_PRIMITIVE_GRAPH: {
            int avail_space = main_node->primitive_num;
            int rest_data = mgs_num;
            int vertex_num;
            if (main_node->primitive_type == BASIC_PRIMITIVE_POLYGON)
            {
                basic_polygon_attr_t *bp_attr = static_cast<basic_polygon_attr_t *>(main_node->graph_primitive_attr);
                vertex_num = bp_attr->vertex_num;
            }
            else
            {
                basic_graph_attr_t *bg_attr = static_cast<basic_graph_attr_t *>(main_node->graph_primitive_attr);
                vertex_num = bg_attr->vertex_num;
            }
            basic_vertex_set_t *b_vertext_set = static_cast<basic_vertex_set_t *>(main_node->graph_primitive_head);
            while (avail_space > 0 && b_vertext_set != nullptr)
            {
                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));
                memcpy(&b_vertext_set->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_vertext_set->local_attr.score, cur_md + sizeof(meta_attr::label_index),
                       sizeof(meta_attr::score));
                for (int count = 0; count < vertex_num; count++)
                {
                    uint16_t xy[2] = {0};
                    memcpy(&xy, temp_data_p, sizeof(xy));
                    b_vertext_set->set_vertex(meta_point_t(xy[0], xy[1]));
                    temp_data_p = temp_data_p + sizeof(meta_point_t::loc) / sizeof(meta_point_t::loc.first);
                }
                cur_md = cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) +
                         sizeof(meta_point_t::loc) * vertex_num;
                b_vertext_set = static_cast<basic_vertex_set_t *>(b_vertext_set->local_attr.next_prim_node);
                avail_space--;
                rest_data--;
            }
            b_vertext_set = static_cast<basic_vertex_set_t *>(main_node->graph_primitive_tail);
            while (rest_data > 0)
            {
                basic_vertex_set_t *b_vertext_set_temp = new basic_vertex_set_t;

                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));
                memcpy(&b_vertext_set_temp->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_vertext_set_temp->local_attr.score, cur_md + sizeof(meta_attr::label_index),
                       sizeof(meta_attr::score));
                for (int count = 0; count < vertex_num; count++)
                {
                    uint16_t xy[2] = {0};
                    memcpy(&xy, temp_data_p, sizeof(xy));
                    b_vertext_set_temp->set_vertex(meta_point_t(xy[0], xy[1]));
                    temp_data_p = temp_data_p + sizeof(meta_point_t::loc) / sizeof(meta_point_t::loc.first);
                }
                b_vertext_set_temp->local_attr.next_prim_node = nullptr;
                b_vertext_set_temp->local_attr.main_node = main_node;
                b_vertext_set_temp->local_attr.shared_attr = main_node->local_attr;
                cur_md = cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) +
                         sizeof(meta_point_t::loc) * vertex_num;
                b_vertext_set->local_attr.next_prim_node = b_vertext_set_temp;
                b_vertext_set = b_vertext_set_temp;
                rest_data--;
            }
            main_node->graph_primitive_tail = b_vertext_set;
            main_node->primitive_num = mgs_num;
            break;
        }
        break;
        case BASIC_PRIMITIVE_CIRCLE: {
            int avail_space = main_node->primitive_num;
            int rest_data = mgs_num;
            basic_circle_t *b_circle = static_cast<basic_circle_t *>(main_node->graph_primitive_head);
            while (avail_space > 0 && b_circle != nullptr)
            {
                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));
                memcpy(&b_circle->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_circle->local_attr.score, cur_md + sizeof(meta_attr::label_index), sizeof(meta_attr::score));
                uint16_t xy[2] = {0};
                memcpy(&xy, temp_data_p, sizeof(xy));
                b_circle->center.set_loc(xy[0], xy[1]);
                b_circle->radius = temp_data_p[2];
                cur_md = cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) +
                         sizeof(basic_circle_t::radius) + sizeof(basic_circle_t::center);
                b_circle = static_cast<basic_circle_t *>(b_circle->local_attr.next_prim_node);
                avail_space--;
                rest_data--;
            }
            b_circle = static_cast<basic_circle_t *>(main_node->graph_primitive_tail);
            while (rest_data > 0)
            {
                basic_circle_t *b_circle_temp = new basic_circle_t;
                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));
                memcpy(&b_circle_temp->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_circle_temp->local_attr.score, cur_md + sizeof(meta_attr::label_index),
                       sizeof(meta_attr::score));
                uint16_t xy[2] = {0};
                memcpy(&xy, temp_data_p, sizeof(xy));
                b_circle_temp->center.set_loc(xy[0], xy[1]);
                b_circle_temp->radius = temp_data_p[2];
                b_circle_temp->local_attr.next_prim_node = nullptr;
                b_circle_temp->local_attr.main_node = main_node;
                b_circle_temp->local_attr.shared_attr = main_node->local_attr;
                cur_md = cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) +
                         sizeof(basic_circle_t::radius) + sizeof(basic_circle_t::center);
                b_circle->local_attr.next_prim_node = b_circle_temp;
                b_circle = b_circle_temp;
                rest_data--;
            }
            main_node->graph_primitive_tail = b_circle;
            main_node->primitive_num = mgs_num;
        }
        break;
        case BASIC_PRIMITIVE_ELLIPSE: {
            int avail_space = main_node->primitive_num;
            int rest_data = mgs_num;
            basic_ellipse_t *b_ellipse = static_cast<basic_ellipse_t *>(main_node->graph_primitive_head);
            while (avail_space > 0 && b_ellipse != nullptr)
            {
                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));
                memcpy(&b_ellipse->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_ellipse->local_attr.score, cur_md + sizeof(meta_attr::label_index), sizeof(meta_attr::score));
                uint16_t points[4] = {0};
                memcpy(&points, temp_data_p, sizeof(points));
                b_ellipse->center.set_loc(points[0], points[1]);
                b_ellipse->x_radius = points[2];
                b_ellipse->y_radius = points[3];
                b_ellipse->rotate_theta = (reinterpret_cast<uint32_t *>(cur_md))[3];
                cur_md = cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) +
                         sizeof(basic_ellipse_t::center) + sizeof(basic_ellipse_t::x_radius) +
                         sizeof(basic_ellipse_t::y_radius) + sizeof(basic_ellipse_t::rotate_theta);
                b_ellipse = static_cast<basic_ellipse_t *>(b_ellipse->local_attr.next_prim_node);
                avail_space--;
                rest_data--;
            }
            b_ellipse = static_cast<basic_ellipse_t *>(main_node->graph_primitive_tail);
            while (rest_data > 0)
            {
                basic_ellipse_t *b_ellipse_temp = new basic_ellipse_t;
                uint16_t *temp_data_p =
                    reinterpret_cast<uint16_t *>(cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score));
                memcpy(&b_ellipse_temp->local_attr.label_index, cur_md, sizeof(meta_attr::label_index));
                memcpy(&b_ellipse_temp->local_attr.score, cur_md + sizeof(meta_attr::label_index),
                       sizeof(meta_attr::score));
                uint16_t points[4] = {0};
                memcpy(&points, temp_data_p, sizeof(points));
                b_ellipse_temp->center.set_loc(points[0], points[1]);
                b_ellipse_temp->x_radius = points[2];
                b_ellipse_temp->y_radius = points[3];
                b_ellipse_temp->rotate_theta = (reinterpret_cast<float *>(cur_md))[3];
                b_ellipse_temp->local_attr.next_prim_node = nullptr;
                b_ellipse_temp->local_attr.main_node = main_node;
                b_ellipse_temp->local_attr.shared_attr = main_node->local_attr;
                cur_md = cur_md + sizeof(meta_attr::label_index) + sizeof(meta_attr::score) +
                         sizeof(basic_ellipse_t::center) + sizeof(basic_ellipse_t::x_radius) +
                         sizeof(basic_ellipse_t::y_radius) + sizeof(basic_ellipse_t::rotate_theta);
                b_ellipse->local_attr.next_prim_node = b_ellipse_temp;
                b_ellipse = b_ellipse_temp;
                rest_data--;
            }
            main_node->graph_primitive_tail = b_ellipse;
            main_node->primitive_num = mgs_num;
        }
        break;
        default:
            break;
        }
        main_node = main_node->main_node;
    }

    return 0;
}

int print_global_tasks_map(const std::unordered_map<std::string, Graph_List_t *> &global_tasks_map)
{
    for (const auto &[task_name, gl_head] : global_tasks_map)
    {
        printf("%s\n", std::string(100, '*').c_str());
        printf("\t%s has %d main node!\t\n", task_name.c_str(), gl_head->main_point_num);
        main_node_t *main_node = gl_head->main_node;
        int j = 0;
        for (int i = 0; main_node; i++, main_node = main_node->main_node)
        {
            printf("\t type %d has %d primitive nodes!\n", main_node->primitive_type, main_node->primitive_num);
        }
        for (const std::string &label_ele : gl_head->graph_label_info.labels_ele)
        {
            if (!(j % 5))
                printf("\t\n");
            printf("%s\t", label_ele.c_str());
        }
        printf("\n%s", std::string(100, '*').c_str());
    }
    return 0;
}

int uninit_graph_list_spec(std::unordered_map<std::string, Graph_List_t *> global_tasks_map,
                           const std::string &task_name)
{
    int ret = 0;
    auto it = global_tasks_map.find(task_name);
    if (it == global_tasks_map.end())
    {
        ret = -2;
        printf("[ERROR]: %s task doesn't exist!\n", task_name.c_str());
        return ret;
    }
    Graph_List_t *gl_head = it->second;
    ret = destory_graph_list(gl_head);
    if (ret)
        printf("[ERROR]: The process of freeing %s resource exists problems, check it\n", task_name.c_str());
    else
        printf("[INFO]: Free %s task resource successfully\n", task_name.c_str());

    return ret;
}

int uninit_global_tasks_map(std::unordered_map<std::string, Graph_List_t *> global_tasks_map)
{
    int ret = 0;
    for (const auto &[task_name, gl_head] : global_tasks_map)
    {
        ret = destory_graph_list(gl_head);
        if (ret)
            printf("[ERROR]: The process of freeing %s resource exists problems, check it\n", task_name.c_str());
        else
            printf("[INFO]: Free %s task resource successfully\n", task_name.c_str());
    }
    return ret;
}

} // namespace MetaGraph