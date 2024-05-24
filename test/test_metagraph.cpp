#include "metagraph.h"
#include "opencv2/opencv.hpp"
#include <yaml-cpp/yaml.h>

#define OPENPOSE_CONFIG_PATH "/home/psf/work_space/scripts_projects/bpu_depoly/temp/configs/openpose_draw.yaml"

#define TASK_CONFIG_PATH "/home/psf/work_space/scripts_projects/bpu_depoly/temp/configs/models_task.yaml"

int simulate_init_metagraph_map(std::map<std::string, MetaGraph::Graph_List_t *> &global_tasks_map)
{
    YAML::Node models_task_file = YAML::LoadFile(TASK_CONFIG_PATH);
    YAML::Node task_list_node = models_task_file["algo_tasks_list"];
    std::vector<std::string> tasks_list;
    if (task_list_node.IsSequence())
    {
        for (const YAML::Node &node : task_list_node)
        {
            tasks_list.emplace_back(node.as<std::string>());
        }
    }
    MetaGraph::init_global_tasks_map(global_tasks_map, tasks_list);
    return 0;
}

int simulate_update_metagraph_map(const std::string &task_name,
                                  std::map<std::string, MetaGraph::Graph_List_t *> &global_tasks_map)
{
    // cv::FileStorage mg_config(OPENPOSE_CONFIG_PATH, cv::FileStorage::READ);

    YAML::Node mg_config = YAML::LoadFile(OPENPOSE_CONFIG_PATH);
    YAML::Emitter out;
    out << mg_config;
    std::string config_str = out.c_str();
    // cv::FileNode task_node = mg_config.root();
    // task_node >> config_str;
    return MetaGraph::update_global_tasks_map(config_str, global_tasks_map, task_name);
}

int simulate_construct_basic_primitives(uint8_t *metadata,
                                        std::map<std::string, MetaGraph::Graph_List_t *> global_tasks_map)
{
    int ret = 0;
    ret = MetaGraph::construct_basic_primitives(metadata, global_tasks_map);
    return ret;
}

int simulate_draw_metagraph(const std::map<std::string, MetaGraph::Graph_List_t *> &global_tasks_map, int task_id,
                            uint8_t *img_data, int width, int height)
{
    auto it = std::next(global_tasks_map.begin(), task_id);
    if (it == global_tasks_map.end())
    {
        printf("[ERROR]: wrong task_id!\n");
        return -1;
    }
    MetaGraph::Graph_List_t *gl_head = it->second;
    MetaGraph::main_node_t *main_node = gl_head->main_node;
    cv::Mat img = cv::Mat(height, width, CV_8UC3, img_data);
    int fontFace = cv::FONT_HERSHEY_TRIPLEX;
    int thickness = 5;
    int line_thickness = 5;
    for (int i = 0; i < gl_head->main_point_num && main_node != nullptr; i++)
    {
        switch (main_node->primitive_type)
        {
        case BASIC_PRIMITIVE_NONE: {
            MetaGraph::meta_attr_t *m_attr = static_cast<MetaGraph::meta_attr_t *>(main_node->graph_primitive_head);
            int label_index = m_attr->label_index;
            std::string text = gl_head->graph_label_info.labels_ele[label_index];
            cv::Size text_size = cv::getTextSize(text, fontFace, m_attr->shared_attr->value, thickness, 0);
            cv::Point text_loc(width - text_size.width - 10, text_size.height + 10);
            cv::putText(img, text, text_loc, fontFace, m_attr->shared_attr->value,
                        cv::Scalar(m_attr->shared_attr->color_u[0], m_attr->shared_attr->color_u[1],
                                   m_attr->shared_attr->color_u[2], m_attr->shared_attr->color_u[3]),
                        thickness);
        }
        break;
        case BASIC_PRIMITIVE_POINT: {
        }
        break;
        case BASIC_PRIMITIVE_LINE: {
        }
        break;
        case BASIC_PRIMITIVE_RECT: {
        }
        break;
        case BASIC_PRIMITIVE_POLYGON: {
        }
        break;
        case BASIC_PRIMITIVE_GRAPH: {
            int bp_num = main_node->primitive_num;
            MetaGraph::basic_vertex_set_t *bp_comp =
                static_cast<MetaGraph::basic_vertex_set *>(main_node->graph_primitive_head);
            MetaGraph::basic_graph_attr_t *graph_attr =
                static_cast<MetaGraph::basic_graph_attr_t *>(main_node->graph_primitive_attr);
            MetaGraph::shared_attr *shared_comp_attr =
                static_cast<MetaGraph::shared_attr *>(bp_comp->local_attr.shared_attr);
            cv::Scalar color = cv::Scalar(shared_comp_attr->color_u[0], shared_comp_attr->color_u[1],
                                          shared_comp_attr->color_u[2], shared_comp_attr->color_u[3]);
            int point_radius = 2;
            while (bp_num--)
            {
                std::vector<cv::Point> points(graph_attr->vertex_num);
                for (int16_t i = 0; i < graph_attr->vertex_num; i++)
                    points[i] = cv::Point(bp_comp->vertex_set[i].loc.first, bp_comp->vertex_set[i].loc.second);
                for (const auto &point : points)
                    cv::circle(img, point, point_radius, color, thickness);
                for (const auto &connection : graph_attr->connection_set)
                {
                    if (points[connection.first].x == -1 || points[connection.first].y == -1 ||
                        points[connection.second].x == -1 || points[connection.second].y == -1)
                        continue;
                    cv::line(img, points[connection.first], points[connection.second], color, line_thickness);
                }
                bp_comp = static_cast<MetaGraph::basic_vertex_set *>(bp_comp->local_attr.next_prim_node);
            }
        }
        break;
        case BASIC_PRIMITIVE_CIRCLE: {
        }
        break;
        case BASIC_PRIMITIVE_ELLIPSE: {
        }
        break;
        default:
            break;
        }
    }
    cv::imshow("Image results", img);
    cv::waitKey(0);
    return 0;
}

int main(int argc, char const *argv[])
{
    uint8_t res_array[] = {
        0,   1,   1, 0,   0,   0,   0,   14,  170, 125, 65,  132, 3, 160, 0, 152, 3,   4,   1,   12,  3,   24,
        1,   188, 2, 184, 1,   148, 2,   108, 2,   16,  4,   240, 0, 156, 4, 144, 1,   0,   5,   28,  2,   72,
        3,   204, 1, 255, 255, 255, 255, 255, 255, 255, 255, 212, 3, 204, 1, 255, 255, 255, 255, 255, 255, 255,
        255, 92,  3, 120, 0,   152, 3,   120, 0,   72,  3,   140, 0, 192, 3, 140, 0,   0,   0,   0,   0,   0,
        0,   0,   0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0,   0, 0,   0,   0,   0,   0,   0,   0,
        0,   0,   0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0,   0, 0,   0,   0,   0,   0,   0,   0,
        0,   0,   0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0,   0, 0,   0,   0,   0,   0,   0,   0,
        0,   0,   0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0,   0, 0,   0,   0,   0};

    std::map<std::string, MetaGraph::Graph_List_t *> global_tasks_map;
    std::string task_name = "openpose";
    uint8_t *img_data = new uint8_t[1920 * 1080 * 3];
    memset(img_data, 0, sizeof(uint8_t) * 1920 * 1080 * 3);
    simulate_init_metagraph_map(global_tasks_map);
    simulate_update_metagraph_map(task_name, global_tasks_map);
    simulate_construct_basic_primitives(res_array, global_tasks_map);

    simulate_draw_metagraph(global_tasks_map, res_array[0], img_data, 1920, 1080);
    delete[] img_data;
    return 0;
}
