// TODO: add 
#include <filesystem>

#include "log_base.h"
#include "plot_data_format.h"

#define FRAME_BUFFER_SIZE 2
#define RESULT_BUFFER_SIZE 10
#define TASK_CONFIG_DIR "/configs/"

typedef struct task_para
{
    int frame_buffer_size;
    int result_buffer_size;
    std::string task_config_file;
} task_para_t;

static task_para_t task_para_hndl_g = {FRAME_BUFFER_SIZE, RESULT_BUFFER_SIZE, TASK_CONFIG_DIR};

typedef enum PRIMITIVE_TYPE
{
    PRIM_NONE = 0,
    PRIM_POINT = 1,
    PRIM_LINE = 2,
    PRIM_RECT = 3,
    PRIM_POLYGON = 4,
    PRIM_GRAPH = 5,
    PRIM_CIRCLE = 6,
    PRIM_ELLIPSE = 7,
} PRIMITIVE_TYPE_T;

static int mapping_points_set_node(points_2d_comp_t *comp_hndl, uint8_t *dst_buf, int avail_space, int comp_num,
                                   int &real_comp_num)
{
    int data_len = sizeof(weight_comp) + sizeof(point_2d_t) * comp_hndl->points_num;
    int cur_avail_space = avail_space;
    points_2d_comp_t *cur_comp_hndl = comp_hndl;
    int count = real_comp_num;
    while (cur_comp_hndl && count < comp_num)
    {
        cur_avail_space -= data_len;
        if (cur_avail_space < 0)
        {
            LOGW_print("Data length exceeds available space %d byte!\n", avail_space);
            break;
        }
        memcpy(dst_buf, &cur_comp_hndl->weight, data_len);
        count++;
        cur_comp_hndl = cur_comp_hndl->next_pcomp;
        dst_buf += data_len;
    }
    real_comp_num = count;
    return data_len * count;
}

static int mapping_circle_node(circle_comp_t *comp_hndl, uint8_t *dst_buf, int avail_space, int comp_num,
                               int &real_comp_num)
{
    int data_len = sizeof(circle_comp_t);
    circle_comp_t *cur_comp_hndl = comp_hndl;
    int cur_avail_space = avail_space;
    int count = real_comp_num;
    while (cur_comp_hndl && count < comp_num)
    {
        cur_avail_space -= data_len;
        if (cur_avail_space < 0)
        {
            LOGW_print("Data length exceeds available space %d byte!\n", avail_space);
            break;
        }
        memcpy(dst_buf, &comp_hndl->weight, data_len);
        count++;
        cur_comp_hndl = cur_comp_hndl->next_ccomp;
        dst_buf += data_len;
    }
    real_comp_num = count;

    return data_len * count;
}

static int mapping_ellipse_node(ellipse_comp_t *comp_hndl, uint8_t *dst_buf, int avail_space, int comp_num,
                                int &real_comp_num)
{
    int data_len = sizeof(ellipse_comp_t);
    ellipse_comp_t *cur_comp_hndl = comp_hndl;
    int cur_avail_space = avail_space;
    int count = real_comp_num;
    while (cur_comp_hndl && count < comp_num)
    {
        cur_avail_space -= data_len;
        if (cur_avail_space < 0)
        {
            LOGW_print("Data length exceeds available space %d byte!\n", avail_space);
            break;
        }
        memcpy(dst_buf, &comp_hndl->weight, data_len);
        count++;
        cur_comp_hndl = cur_comp_hndl->next_ecomp;
        dst_buf += data_len;
    }
    real_comp_num = count;
    return data_len * count;
}

static int destory_transit_node(transit_node_t *tst_node)
{
    int ret = 0;
    if (tst_node == nullptr)
    {
        LOGD_print("destory_transit_node successfully!\n");
        return ret;
    }

    switch (tst_node->comp_type)
    {
    case POINTS_2D: {
        points_2d_comp_t *p_comp = static_cast<points_2d_comp_t *>(tst_node->comp_node_head);
        while (p_comp)
        {
            points_2d_comp_t *p_comp_temp = p_comp;
            p_comp = p_comp->next_pcomp;
            delete p_comp_temp;
        }
    }
    break;
    case CIRCLE: {
        circle_comp_t *c_comp = static_cast<circle_comp_t *>(tst_node->comp_node_head);
        while (c_comp)
        {
            circle_comp_t *c_comp_temp = c_comp;
            c_comp = c_comp->next_ccomp;
            delete c_comp_temp;
        }
    }
    break;
    case ELLIPSE: {
        ellipse_comp_t *e_comp = static_cast<ellipse_comp_t *>(tst_node->comp_node_head);
        while (e_comp)
        {
            ellipse_comp_t *e_comp_temp = e_comp;
            e_comp = e_comp->next_ecomp;
            delete e_comp_temp;
        }
    }
    break;
    default:
        printf("[ERROR]: Failed to free the type %d of primitive!\n", tst_node->comp_type);
        ret = -1;
        break;
    }
    destory_transit_node(tst_node->next_tnode);
    delete tst_node;
    return ret;
}

static int mapping_comp_list(transit_node_t *tst_node, uint8_t *dst_buf, int comp_type_num, int avail_space)
{
    uint8_t *cur_dst_buf = dst_buf + comp_type_num;
    transit_node_t *cur_tnode = tst_node;
    int cur_avail_space = avail_space - comp_type_num;
    int count = 0;
    while (cur_tnode)
    {
        int data_offset = 0;
        int real_comp_num = 0;
        switch (cur_tnode->comp_type)
        {
        case POINTS_2D:
            data_offset = mapping_points_set_node(static_cast<points_2d_comp_t *>(tst_node->comp_node_head),
                                                  cur_dst_buf, cur_avail_space, tst_node->comp_num, real_comp_num);
            dst_buf[count] = real_comp_num;
            break;
        case CIRCLE:
            data_offset = mapping_circle_node(static_cast<circle_comp_t *>(tst_node->comp_node_head), cur_dst_buf,
                                              cur_avail_space, tst_node->comp_num, real_comp_num);
            dst_buf[count] = real_comp_num;
            break;
        case ELLIPSE:
            data_offset = mapping_ellipse_node(static_cast<ellipse_comp_t *>(tst_node->comp_node_head), cur_dst_buf,
                                               cur_avail_space, tst_node->comp_num, real_comp_num);
            dst_buf[count] = real_comp_num;
            break;
        default:
            LOGW_print("Unspported comp type!\n");
            break;
        }
        count++;
        cur_dst_buf += data_offset;
        cur_avail_space -= data_offset;
        cur_tnode = cur_tnode->next_tnode;
        if (comp_type_num < count)
        {
            LOGW_print("The num of real transit_node is greater than the pre-allocated transit_node!\n");
            break;
        }
    }
    LOGD_print("The current remaining space is %d\n", cur_avail_space);
    return avail_space - cur_avail_space;
}

main_transit_node_t *init_mt_stream()
{
    if (!std::filesystem::exists(task_para_hndl_g.task_config_file))
    {
        LOGE_print("Task config file [%s] doesn't exist!\n", task_para_hndl_g.task_config_file.c_str());
        return nullptr;
    }
    YAML::Node task_config = YAML::LoadFile(task_para_hndl_g.task_config_file);
    YAML::Node task_bp_info = task_config["basic_primitives"];
    int bp_num = task_bp_info["glob_attr"] ? -1 : 0;
    bp_num += task_bp_info.size();
    main_transit_node_t *mtst_node =
        (main_transit_node_t *)malloc(sizeof(main_transit_node_t) + sizeof(transit_node_t *) * bp_num);
    mtst_node->avail_transit_node_num = bp_num;
    mtst_node->transit_node_num = bp_num;
    for (int i = 1; i <= bp_num; i++)
    {
        std::string bp_name = std::string("bp") + std::to_string(i);
        YAML::Node bp_node = task_bp_info[bp_name];
        PRIMITIVE_TYPE_T comp_type = static_cast<PRIMITIVE_TYPE_T>(bp_node["Comp_type"].as<int>());
        transit_node_t *t_node_temp = new transit_node_t;
        int points_num = 0;
        t_node_temp->comp_num = 0;
        t_node_temp->avail_space = 1;
        switch (comp_type)
        {
        case PRIM_NONE: {
            t_node_temp->comp_type = COMP_TYPE_T::POINTS_2D;
            points_2d_comp_t *p_comp =
                (points_2d_comp_t *)malloc(sizeof(points_2d_comp_t) + sizeof(point_2d_t) * points_num);
            p_comp->points_num = points_num;
            t_node_temp->comp_node_head = p_comp;
        }
        break;
        case PRIM_POINT: {
            points_num = 1;
            t_node_temp->comp_type = COMP_TYPE_T::POINTS_2D;
            points_2d_comp_t *p_comp =
                (points_2d_comp_t *)malloc(sizeof(points_2d_comp_t) + sizeof(point_2d_t) * points_num);
            p_comp->points_num = points_num;
            t_node_temp->comp_node_head = p_comp;
        }
        break;
        case PRIM_LINE:
        case PRIM_RECT: {
            points_num = 2;
            t_node_temp->comp_type = COMP_TYPE_T::POINTS_2D;

            points_2d_comp_t *p_comp =
                (points_2d_comp_t *)malloc(sizeof(points_2d_comp_t) + sizeof(point_2d_t) * points_num);
            p_comp->points_num = points_num;
            t_node_temp->comp_node_head = p_comp;
        }
        break;
        case PRIM_POLYGON: {
            points_num = bp_node["Edges_num"].as<int>() + 1;
            t_node_temp->comp_type = COMP_TYPE_T::POINTS_2D;

            points_2d_comp_t *p_comp =
                (points_2d_comp_t *)malloc(sizeof(points_2d_comp_t) + sizeof(point_2d_t) * points_num);
            p_comp->points_num = points_num;
            t_node_temp->comp_node_head = p_comp;
        }
        break;
        case PRIM_GRAPH: {
            points_num = bp_node["Points_num"].as<int>();
            t_node_temp->comp_type = COMP_TYPE_T::POINTS_2D;

            points_2d_comp_t *p_comp =
                (points_2d_comp_t *)malloc(sizeof(points_2d_comp_t) + sizeof(point_2d_t) * points_num);
            p_comp->points_num = points_num;
            t_node_temp->comp_node_head = p_comp;
        }
        break;
        case PRIM_CIRCLE:
            t_node_temp->comp_type = COMP_TYPE_T::CIRCLE;
            t_node_temp->comp_node_head = malloc(sizeof(circle_comp_t));
            break;
        case PRIM_ELLIPSE:
            t_node_temp->comp_type = COMP_TYPE_T::ELLIPSE;
            t_node_temp->comp_node_head = malloc(sizeof(ellipse_comp_t));
            break;
        default:
            LOGE_print("error comp type %d!\n", comp_type);
            delete t_node_temp;
            continue;
        }
        t_node_temp->avail_space = 1;
        t_node_temp->comp_num = 0;
        t_node_temp->next_tnode = nullptr;
        t_node_temp->comp_node_tail = t_node_temp->comp_node_head;
        mtst_node->tnode_list[i - 1] = t_node_temp;
    }
    return mtst_node;
}

int update_transit_node(transit_node_t *tst_node, int comp_num)
{
    int needed_node_num = comp_num - tst_node->avail_space;
    tst_node->comp_num = comp_num;
    if (needed_node_num < 1)
    {
        LOGD_print("tst_node doesn't need to be updated!\n");
        return 0;
    }

    switch (tst_node->comp_type)
    {
    case POINTS_2D: {
        int points_num = (static_cast<points_2d_comp_t *>(tst_node->comp_node_head))->points_num;
        points_2d_comp_t *p_comp_tail = static_cast<points_2d_comp_t *>(tst_node->comp_node_tail);

        for (int i = 0; i < needed_node_num; i++)
        {
            points_2d_comp_t *points_comp =
                (points_2d_comp_t *)malloc(sizeof(points_2d_comp_t) + sizeof(point_2d_t) * points_num);
            points_comp->points_num = points_num;
            points_comp->next_pcomp = p_comp_tail->next_pcomp;
            p_comp_tail->next_pcomp = points_comp;
            p_comp_tail = points_comp;
            tst_node->avail_space++;
        }
        tst_node->comp_node_tail = p_comp_tail;
    }

    break;
    case CIRCLE: {
        circle_comp_t *c_comp_tail = static_cast<circle_comp_t *>(tst_node->comp_node_tail);
        for (int i = 0; i < needed_node_num; i++)
        {
            circle_comp_t *circle_comp = (circle_comp_t *)malloc(sizeof(circle_comp_t));
            circle_comp->next_ccomp = c_comp_tail->next_ccomp;
            c_comp_tail->next_ccomp = circle_comp;
            c_comp_tail = circle_comp;
            tst_node->avail_space++;
        }
        tst_node->comp_node_tail = c_comp_tail;
    }
    break;
    case ELLIPSE: {
        ellipse_comp_t *e_comp_tail = static_cast<ellipse_comp_t *>(tst_node->comp_node_tail);
        for (int i = 0; i < needed_node_num; i++)
        {
            ellipse_comp_t *ellipse_comp = (ellipse_comp_t *)malloc(sizeof(ellipse_comp_t));
            ellipse_comp->next_ecomp = e_comp_tail->next_ecomp;
            e_comp_tail->next_ecomp = ellipse_comp;
            e_comp_tail = ellipse_comp;
            tst_node->comp_node_tail = ellipse_comp;
            tst_node->avail_space++;
        }
        tst_node->comp_node_tail = e_comp_tail;
    }
    break;
    default:
        LOGE_print("Wrong comp type of transit node!\n");
        return -1;
    }

    LOGD_print("tst_node updates completely!\n");
    return 0;
}

int destory_mt_stream(main_transit_node_t *mtst_node)
{
    if (!mtst_node)
    {
        LOGD_print("main transit_node is null!\n");
        return -1;
    }
    int ret = 0;
    ret = destory_transit_node(mtst_node->tnode_list[0]);
    delete mtst_node;
    return ret;
}

#if Demo
int mapping_result(task_hndl_t *task_info, main_transit_node_t *mt_node, mtask_hndl_t handle)
{
    if (mt_node->avail_transit_node_num < 1)
    {
        LOGD_print("No transit node available\n");
        return -1;
    }
    int ret = 0;
    uint16_t data_len = 0;
    model_task_t *mt_hndl = reinterpret_cast<model_task_t *>(handle);
    model_t *model_hndl = mt_hndl->model_handle;
    auto timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    model_hndl->m_src_data_queue[task_info->src_img_info->data_info.src_data_info.frame_id]->time_stamp_infer =
        static_cast<uint64_t>(timestamp);
    cam_data_t *cam_data =
        mt_hndl->model_handle->m_src_data_queue[task_info->src_img_info->data_info.src_data_info.frame_id];
    ret = pub_node_acquire_wbuf(mt_hndl->metadata_pub_node, &cam_data->metadata_info.metadata_raw, 1);
    if (ret < 1)
    {
        LOGW_print("%s acquire metadata buf failed!", task_info->task_name);
        return -2;
    }
    uint8_t *metadata_buf = static_cast<uint8_t *>(cam_data->metadata_info.metadata_raw.ptr);
    uint8_t *metadata_data_buf = metadata_buf + RESULT_DATA_HEAD_LEN;
    metadata_buf[2] = (uint8_t)(cam_data->cam_frame_id >> 8);
    metadata_buf[3] = (uint8_t)(cam_data->cam_frame_id & 0xff);
    metadata_buf[4] = static_cast<uint8_t>(model_hndl->input_src);
    metadata_buf[5] = static_cast<uint8_t>(task_info->model_id);
    metadata_buf[6] = static_cast<uint8_t>(mt_node->transit_node_num & 0xff);

    data_len = mapping_comp_list(mt_node->tnode_list[0], metadata_data_buf, mt_node->avail_transit_node_num,
                                 RESULT_METADATA_LEN - RESULT_DATA_HEAD_LEN);
    data_len = data_len == mt_node->avail_transit_node_num ? 0 : data_len;
    metadata_buf[0] = (uint8_t)(data_len >> 8);
    metadata_buf[1] = (uint8_t)(data_len & 0xff);

    LOGI_print("results_len = %u\n", data_len);
#if 0
    {
        int16_t *tst_md = reinterpret_cast<int16_t *>(metadata_buf);
        LOGD_print("\n****mapping result****\n");
        LOGI_print("cam_frame_id = %u\n", cam_data->cam_frame_id);
        for (int k = 0; k < data_len; k++)
        {
            LOGI_print("%d\n", tst_md[k]);
        }
        LOGD_print("\n****mapping result****\n");
    }
#endif

#if 1
    uint8_t *p_metadata_buf = metadata_buf + 5;
    printf("\n**** mapping result begin ****\n");
    for (int i = 0; i < RESULT_METADATA_LEN - 5; i++)
    {
        if (!(i % 50))
            printf("\n");
        printf("%u, ", p_metadata_buf[i]);
    }
    printf("\n**** mapping result end ****\n");
#endif
    pub_node_release_wbuf(mt_hndl->metadata_pub_node, &cam_data->metadata_info.metadata_raw, 1);
    LOGD_print("release bpu_pre_alloc_buf_id:%d\n", model_hndl->cur_head_raw_data);

    model_hndl->cur_head_raw_data = (model_hndl->cur_head_raw_data + 1) % task_para_hndl_g.frame_buffer_size;
    return ret > 0 ? 0 : ret;
}

#endif