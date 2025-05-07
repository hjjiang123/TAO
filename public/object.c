#include "object.h"

// utility functions
int mask_to_prefix_length(unsigned int mask){
    int prefixLength = 0;
    while (mask << prefixLength != 0) {
        prefixLength++;
        // printf("len:%d ",prefixLength);
        // printf("mask:%u\n",(mask << prefixLength)>>prefixLength);
    }
    return prefixLength;
}

//完全对象交集集合
InterObject **complete_interset_objects; // S

//对象差集集合
DifferObject **differ_objects; // D
 
//初始化
void init_objects(){
    complete_interset_objects = (InterObject **)malloc(MAX_INTERSECTION_NUM*sizeof(InterObject *));
    differ_objects = (DifferObject **)malloc(MAX_INTERSECTION_NUM*sizeof(DifferObject *));
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        complete_interset_objects[i]=NULL;
        differ_objects[i]=NULL;
    }
}  


//begin...对象交集操作
bool equals_intersection_object(InterObject *obj1, InterObject *obj2)
{
    if( obj1->direction == 0 && obj2->direction==0 &&
        obj1->src_ip == obj2->src_ip && 
        obj1->src_mask == obj2->src_mask && 
        obj1->dst_ip == obj2->dst_ip && 
        obj1->dst_mask == obj2->dst_mask && 
        obj1->src_port == obj2->src_port && 
        obj1->dst_port == obj2->dst_port && 
        obj1->protocol == obj2->protocol){
        return true;
    }else if(obj1->direction == 1 && obj2->direction==1 &&
        obj1->src_ip == obj2->src_ip && 
        obj1->src_mask == obj2->src_mask && 
        obj1->dst_ip == obj2->dst_ip && 
        obj1->dst_mask == obj2->dst_mask && 
        obj1->src_port == obj2->src_port && 
        obj1->dst_port == obj2->dst_port && 
        obj1->protocol == obj2->protocol){
        return true;
    }else if(obj1->direction == 1 && obj2->direction==1 &&
        obj1->src_ip == obj2->dst_ip && 
        obj1->src_mask == obj2->dst_mask && 
        obj1->dst_ip == obj2->src_ip && 
        obj1->dst_mask == obj2->src_mask && 
        obj1->src_port == obj2->dst_port && 
        obj1->dst_port == obj2->src_port && 
        obj1->protocol == obj2->protocol){
        return true;
    }
    return false;
}

bool get_intersection_object(InterObject *obj1, InterObject *obj2, InterObject *res_obj3) {
    if(obj1==NULL||obj2==NULL){
        return false;
    }
    if(obj1->direction==0){ 
        if(obj2->direction==0){ // 0 0
            res_obj3->direction = 0;
            // Find intersection of IP and masks
            if(obj1->src_ip < obj2->src_ip){
                if((obj1->src_ip^(~obj1->src_mask))>=(obj2->src_ip^(~obj2->src_mask))){
                    res_obj3->src_ip = obj2->src_ip;
                    res_obj3->src_mask = obj2->src_mask;
                }else{
                    return false;
                }
            }else if(obj2->src_ip < obj1->src_ip){
                if((obj2->src_ip^(~obj2->src_mask))>=(obj1->src_ip^(~obj1->src_mask))){
                    res_obj3->src_ip = obj1->src_ip;
                    res_obj3->src_mask = obj1->src_mask;
                }else{
                    return false;
                }
            }else{
                if((obj2->src_ip^(~obj2->src_mask))>=(obj1->src_ip^(~obj1->src_mask))){
                    res_obj3->src_ip = obj1->src_ip;
                    res_obj3->src_mask = obj1->src_mask;
                }else{
                    res_obj3->src_ip = obj2->src_ip;
                    res_obj3->src_mask = obj2->src_mask;
                }
            }
            if(obj1->dst_ip < obj2->dst_ip){
                if((obj1->dst_ip^(~obj1->dst_mask))>=(obj2->dst_ip^(~obj2->dst_mask))){
                    res_obj3->dst_ip = obj2->dst_ip;
                    res_obj3->dst_mask = obj2->dst_mask;
                }else{
                    return false;
                }
            }else if(obj2->dst_ip < obj1->dst_ip){
                if((obj2->dst_ip^(~obj2->dst_mask))>=(obj1->dst_ip^(~obj1->dst_mask))){
                    res_obj3->dst_ip = obj1->dst_ip;
                    res_obj3->dst_mask = obj1->dst_mask;
                }else{
                    return false;
                }
            }else{
                if((obj2->dst_ip^(~obj2->dst_mask))>=(obj1->dst_ip^(~obj1->dst_mask))){
                    res_obj3->dst_ip = obj1->dst_ip;
                    res_obj3->dst_mask = obj1->dst_mask;
                }else{
                    res_obj3->dst_ip = obj2->dst_ip;
                    res_obj3->dst_mask = obj2->dst_mask;
                }
            }
            if(obj1->src_port == 0){
                res_obj3->src_port = obj2->src_port;
            }else if(obj2->src_port == 0){
                res_obj3->src_port = obj1->src_port;
            }else if(obj1->src_port != obj2->src_port){
                return false;
            }else{
                res_obj3->src_port = obj1->src_port;
            }
            if(obj1->dst_port == 0){
                res_obj3->dst_port = obj2->dst_port;
            }else if(obj2->dst_port == 0){
                res_obj3->dst_port = obj1->dst_port;
            }else if(obj1->dst_port != obj2->dst_port){
                return false;
            }else{
                res_obj3->dst_port = obj1->dst_port;
            }
            if(obj1->protocol == 0){
                res_obj3->protocol = obj2->protocol;
            }else if(obj2->protocol == 0){
                res_obj3->protocol = obj1->protocol;
            }else if(obj1->protocol != obj2->protocol){
                return false;
            }else{
                res_obj3->protocol = obj1->protocol;
            }
        }else{ // 0 1
            obj2->direction = 0;
            if(get_intersection_object(obj1, obj2, res_obj3)){
                obj2->direction = 1;
                return true;
            }
            unsigned int src_ip = obj2->src_ip;      //源地址
            unsigned int src_mask = obj2->src_mask;    //源地址掩码
            unsigned int dst_ip = obj2->dst_ip;     //宿地址
            unsigned int dst_mask = obj2->dst_mask;   //宿地址掩码
            unsigned short src_port = obj2->src_port;  //源地址端口
            unsigned short dst_port = obj2->dst_port; //宿地址端口

            obj2->src_ip = dst_ip;   
            obj2->src_mask = dst_mask;
            obj2->src_port = dst_port;
            obj2->dst_ip = src_ip;
            obj2->dst_mask = src_mask;
            obj2->dst_port = src_port;
            if(get_intersection_object(obj1, obj2, res_obj3)){
                obj2->direction = 1;
                obj2->src_ip = src_ip;   
                obj2->src_mask = src_mask;
                obj2->src_port = src_port;
                obj2->dst_ip = dst_ip;
                obj2->dst_mask = dst_mask;
                obj2->dst_port = dst_port;
                return true;
            }else{
                obj2->direction = 1;
                obj2->src_ip = src_ip;   
                obj2->src_mask = src_mask;
                obj2->src_port = src_port;
                obj2->dst_ip = dst_ip;
                obj2->dst_mask = dst_mask;
                obj2->dst_port = dst_port;
                return false;
            }
            
        }
    }else{ 
        if(obj2->direction==0){ // 1 0
            return get_intersection_object(obj2, obj1, res_obj3);
        }else{ // 1 1
            obj2->direction = 0;
            if(get_intersection_object(obj1, obj2, res_obj3)){
                obj2->direction = 1;
                res_obj3->direction = 1;
                return true;
            }else{
                obj2->direction = 1;
                return false;
            }
            
        }
    }

    return true;
}

bool contain_intersection_object_or_not(InterObject *obj1, InterObject *obj2)
{
    InterObject *res_obj3=(InterObject *)malloc(sizeof(InterObject));
    if(!get_intersection_object(obj1, obj2, res_obj3)){
        free(res_obj3);
        return false;
    }
    if(equals_intersection_object(obj2, res_obj3)){
        free(res_obj3);
        return true;
    }
    free(res_obj3);
    return false;
}

void increase_intersection_object_priority(InterObject *obj)
{
    obj->priority++; 
}

void print_intersection_object(InterObject *obj){
    printf("direction:%u; ",obj->direction);
    printf("src_ip:%d.%d.%d.%d; ",
            (obj->src_ip >> 24) & 0xFF,
            (obj->src_ip >> 16) & 0xFF,
            (obj->src_ip >> 8) & 0xFF,
            obj->src_ip & 0xFF);
    printf("src_mask:%d.%d.%d.%d; ",
            (obj->src_mask >> 24) & 0xFF,
            (obj->src_mask >> 16) & 0xFF,
            (obj->src_mask >> 8) & 0xFF,
            obj->src_mask & 0xFF);
    printf("dst_ip:%d.%d.%d.%d; ",
            (obj->dst_ip >> 24) & 0xFF,
            (obj->dst_ip >> 16) & 0xFF,
            (obj->dst_ip >> 8) & 0xFF,
            obj->dst_ip & 0xFF);
    printf("dst_mask:%d.%d.%d.%d; ",
            (obj->dst_mask >> 24) & 0xFF,
            (obj->dst_mask >> 16) & 0xFF,
            (obj->dst_mask >> 8) & 0xFF,
            obj->dst_mask & 0xFF);
    printf("src_port:%u; ",obj->src_port);
    printf("dst_port:%u; ",obj->dst_port);
    printf("protocol:%u; ",obj->protocol);
    printf("priority:%u;",obj->priority);
    printf("task_ids:");
    for(int i=0;i<obj->priority;i++){
        printf("%d,",obj->task_ids[i]);
    }
    printf("\n");
}

//end...对象交集操作

//begin...完全对象交集集合操作
int find_intersection_object(InterObject *obj)
{
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        if(complete_interset_objects[i]==NULL){
            continue;
        }
        if(equals_intersection_object(complete_interset_objects[i], obj)){
            return i;
        }
    }
    return -1;
}

int find_intersection_object_1(InterObject *obj,InterObject** obj_set, int num)
{
    for(int i=0;i<num;i++){
        if(obj_set[i]==NULL){
            continue;
        }
        if(equals_intersection_object(obj_set[i], obj)){
            return i;
        }
    }
    return -1;
}

int get_all_intersection_object_index(int index_sequence[MAX_INTERSECTION_NUM]){
    int k = 0;
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        if(complete_interset_objects[i]!=NULL){
            index_sequence[k] = i;
            k++;
        }
    }
    return k;
}

InterObject *intersection_object(int index)
{
    if(index<0||index>=MAX_INTERSECTION_NUM){
        return NULL;
    }
    return complete_interset_objects[index];
}

int add_intersection_object(InterObject *obj) 
{
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        if(complete_interset_objects[i]==NULL){
            complete_interset_objects[i]=obj;
            return i;
        }
    }
    return -1;
}

int remove_intersection_object(InterObject *obj)
{
    if(obj == NULL){
        return -1;
    }
    int index = find_intersection_object(obj);
    if(index == -1){
        return -1;
    }
    complete_interset_objects[index] = NULL;
    return index;
}

int get_least_priority_intersection_object(){
    int p = MAX_PRIORITY;
    int j;
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        if(complete_interset_objects[i]==NULL){
            continue;
        }
        if(complete_interset_objects[i]->priority<p){
            p = complete_interset_objects[i]->priority;
            j = i;
        }
    }
    return j;
}
int get_next_least_priority_intersection_object(int cur){
    int p = complete_interset_objects[cur]->priority;
    for(int i=cur+1;i<MAX_INTERSECTION_NUM;i++){
        if(complete_interset_objects[i]==NULL){
            continue;
        }
        if(complete_interset_objects[i]->priority==p){
            return i;
        }
    }
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        if(complete_interset_objects[i]==NULL){
            continue;
        }
        if(complete_interset_objects[i]->priority==p+1){
            return i;
        }
    }
    return -1;
}

int sort_interset_objects(int index_sequence[], InterObject** obj_set, int num) {
    int k = 0;
    for (int i = 0; i < num; i++) {
        if(obj_set[i]!=NULL){
            index_sequence[k] = i;
            k++;
        }
    }

    for (int i = 0; i < k - 1; i++) {
        for (int j = 0; j < k - i - 1; j++) {
            if (obj_set[index_sequence[j]]->priority > obj_set[index_sequence[j + 1]]->priority) {
                int temp = index_sequence[j];
                index_sequence[j] = index_sequence[j + 1];
                index_sequence[j + 1] = temp;
            }
        }
    }
    return k;
}

void end_add_s(){
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        if(complete_interset_objects[i]!=NULL){
            complete_interset_objects[i]->is_new = false;
        }
    }

}

int **produce_global_A(){
    int **A=(int **)malloc(MAX_INTERSECTION_NUM * sizeof(int *));
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        A[i] = (int *)malloc((MAX_TASK_NUM+2) * sizeof(int));
    }
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        for(int j=0;j<MAX_TASK_NUM+1;j++){
            A[i][j] = -1;
        }
        A[i][MAX_TASK_NUM+1] = 0;
    }
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        if(complete_interset_objects[i] == NULL){
            continue;
        }
        for(int j=0;j<complete_interset_objects[i]->priority-1;j++){
            A[i][complete_interset_objects[i]->task_ids[j]] = complete_interset_objects[i]->task_ids[j+1];
        }
        A[i][MAX_TASK_NUM] = complete_interset_objects[i]->task_ids[0];//将input_thread的下一个任务设为该对象的首个任务
        A[i][MAX_TASK_NUM+1] = complete_interset_objects[i]->priority;
    }
    return A;
}

void print_s(){
    printf("-----begin s-----\n");
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        if(complete_interset_objects[i]!=NULL){
            printf("id:%d ",i);
            print_intersection_object(complete_interset_objects[i]);
        }
    }
    printf("-----end s-----\n");
}

//end...完全对象交集集合操作

//begin...对象差集集合操作
void print_difference_object(DifferObject *obj){
    printf("****base object****\n");
    print_intersection_object(obj->base_interset);
    printf("****subset object****\n");
    for(int i=0;i<obj->sub_interset_num;i++){
        print_intersection_object(obj->sub_intersets[i]);
    }
}

void print_d(){
    printf("+++++begin d+++++\n");
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        if(differ_objects[i]!=NULL){
            print_difference_object(differ_objects[i]);
        }
    }
    printf("+++++end d+++++\n");
}

DifferObject *find_difference_object(int index)
{
    if(index<0||index>=MAX_INTERSECTION_NUM){
        return NULL;
    }
    return differ_objects[index];
}

//end...对象差集集合操作

// begin...update algorithm
void produce_diff(InterObject *alpha){
    DifferObject *d_alpha = (DifferObject *)malloc(sizeof(DifferObject));
    d_alpha->base_interset = alpha;
    d_alpha->sub_interset_num = 0;
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        InterObject *alpha_1 = complete_interset_objects[i];
        if(alpha_1==NULL){
            continue;
        }
        if(contain_intersection_object_or_not(alpha, alpha_1)&&!equals_intersection_object(alpha, alpha_1)){
            if(d_alpha->sub_interset_num == MAX_INTERSECTION_NUM_PER_DIFFERENCE){
                //输出错误
                continue;
            }
            d_alpha->sub_intersets[d_alpha->sub_interset_num] = alpha_1;
            d_alpha->sub_interset_num++;
        }
    }
    int index = find_intersection_object(alpha);
    differ_objects[index] = d_alpha;
}

void update_CS_per_intersection_obj(InterObject *origin_obj, int task_id){
    InterObject **temp_c_io = (InterObject**)malloc(sizeof(InterObject *)*MAX_OBJECTS_NUM_PER_TASK); // S'
    for(int i=0;i<MAX_OBJECTS_NUM_PER_TASK;i++){
        temp_c_io[i]=NULL;
    }
    int temp_c_io_num = 0;
    
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        InterObject *alpha = complete_interset_objects[i];
        if(alpha==NULL){
            continue;
        }
        InterObject *res_obj3=(InterObject *)malloc(sizeof(InterObject));
        if(get_intersection_object(origin_obj, alpha, res_obj3)){
            if(find_intersection_object(res_obj3) == -1){
                int pos = find_intersection_object_1(res_obj3, temp_c_io, temp_c_io_num);
                if(pos==-1){
                    DifferObject *d_alpha = differ_objects[i];
                    if(d_alpha->sub_interset_num == MAX_INTERSECTION_NUM_PER_DIFFERENCE){
                        //输出错误
                        continue;
                    }
                    res_obj3->priority = alpha->priority + 1;
                    res_obj3->parrent_obj_id = i;
                    res_obj3->is_new = true;
                    for(int k=0;k<alpha->priority;k++){
                        res_obj3->task_ids[k] = alpha->task_ids[k];
                    }
                    res_obj3->task_ids[res_obj3->priority-1] = task_id;
                    for(int k=0;k<MAX_HOST_NUM;k++){
                        if(alpha->hosts[k]==1){
                            res_obj3->hosts[k] = 1;
                        }
                    }
                    temp_c_io[temp_c_io_num++] = res_obj3;
                    d_alpha->sub_intersets[d_alpha->sub_interset_num++] = res_obj3;
                }else{
                    if(temp_c_io[pos]->priority < alpha->priority + 1){
                        temp_c_io[pos]->priority = alpha->priority + 1;
                        temp_c_io[pos]->parrent_obj_id = i;
                        temp_c_io[pos]->is_new = true;
                        for(int k=0;k<alpha->priority;k++){
                            temp_c_io[pos]->task_ids[k] = alpha->task_ids[k];
                        }
                        temp_c_io[pos]->task_ids[temp_c_io[pos]->priority-1] = task_id;
                        for(int k=0;k<MAX_HOST_NUM;k++){
                            if(alpha->hosts[k]==1){
                                temp_c_io[pos]->hosts[k] = 1;
                            }
                        }
                    }
                    // else{
                    //     temp_c_io[pos]->priority = temp_c_io[pos]->priority + 1;
                    //     temp_c_io[pos]->task_ids[temp_c_io[pos]->priority-1] = task_id;
                    // }
                }
            }else if(contain_intersection_object_or_not(origin_obj,alpha)){
                alpha->priority++;   
                alpha->task_ids[alpha->priority-1] = task_id;
            }
        }
    }
    if(find_intersection_object(origin_obj) == -1&&find_intersection_object_1(origin_obj, temp_c_io, temp_c_io_num) == -1){
        origin_obj->priority = 1;
        origin_obj->parrent_obj_id = -1;
        origin_obj->is_new = true;
        origin_obj->task_ids[0] = task_id;
        for(int k=0;k<MAX_HOST_NUM;k++){
            origin_obj->hosts[k] = 0;
        }
        temp_c_io[temp_c_io_num] = origin_obj;
        temp_c_io_num++;
    }
    for(int i=0;i<temp_c_io_num;i++){
        add_intersection_object(temp_c_io[i]);
    }
    for(int i=0;i<temp_c_io_num;i++){
        produce_diff(temp_c_io[i]);
    }
}


int add_task_all_CS(int task_id, InterObject **origin_obj, int num,DifferObject **do_jk){
    // int do_jk_num = 0;
    
    for(int i=0;i<num;i++){
        update_CS_per_intersection_obj(origin_obj[i], task_id);
    }
    // for(int i=0;i<num;i++){
    //     InterObject *ojk=origin_obj[i];
    //     for(int j=0;j<MAX_INTERSECTION_NUM;j++){
    //         InterObject *alpha_1 = complete_interset_objects[j];
    //         if(alpha_1==NULL){
    //             continue;
    //         }
    //         if(contain_intersection_object_or_not(ojk, alpha_1)){
    //             int index = find_intersection_object(alpha_1);
    //             DifferObject *d_alpha = differ_objects[index];
    //             do_jk[do_jk_num++] = d_alpha;
    //         }
    //     }
    // }
    // return do_jk_num;
    return 0;
    
}

void remove_CS_per_intersection_obj(InterObject *ojk, int task_id){
    int indexes[MAX_INTERSECTION_NUM];
    int idx_num = sort_interset_objects(indexes,complete_interset_objects,MAX_INTERSECTION_NUM);
    for(int i=0;i<idx_num;i++){ 
        bool remove=false; 
        InterObject *alpha = complete_interset_objects[indexes[i]];
        if(alpha==NULL){
            continue;
        }
        if(contain_intersection_object_or_not(ojk, alpha)){
            for(int k=0;k<alpha->priority;k++){
                if(alpha->task_ids[k]==task_id){
                    if(k==alpha->priority-1){
                        alpha->task_ids[k] = 0;
                        break;
                    }
                    for(int l=k;l<alpha->priority-1;l++){
                        alpha->task_ids[l] = alpha->task_ids[l+1];
                    }
                    alpha->task_ids[alpha->priority-1] = 0;
                    break;
                }
            }
            alpha->priority--;
            if(alpha->priority == 0){
                remove_intersection_object(alpha);
                remove=true;
            }else{
                for(int j=0;j<idx_num;j++){
                    InterObject *alpha_1 = complete_interset_objects[indexes[j]];
                    if(alpha_1==NULL){
                        continue;
                    }
                    if(!equals_intersection_object(alpha,alpha_1)&&contain_intersection_object_or_not(alpha_1,alpha)&&alpha_1->priority>=alpha->priority){
                        remove_intersection_object(alpha);
                        remove=true;
                        break;
                    }
                }
            }
            if(remove){
                differ_objects[indexes[i]] = NULL;
                for(int j=0;j<MAX_INTERSECTION_NUM;j++){
                    InterObject *alpha_1 = complete_interset_objects[j];
                    if(alpha_1 == NULL){
                        continue;
                    }
                    if(contain_intersection_object_or_not(alpha_1,alpha)){
                        DifferObject *d_alpha = differ_objects[j];
                        int k;
                        for(k=0;k<d_alpha->sub_interset_num;k++){
                            InterObject *alpha_2 = d_alpha->sub_intersets[k];
                            if(equals_intersection_object(alpha_2, alpha)){
                                break;
                            }
                        }
                        if(k==d_alpha->sub_interset_num){
                            //输出错误
                            continue;
                        }
                        int l;
                        for(l=k;l<d_alpha->sub_interset_num-1;l++){
                            d_alpha->sub_intersets[l] = d_alpha->sub_intersets[l+1];
                        }
                        d_alpha->sub_intersets[l] = NULL;
                        d_alpha->sub_interset_num--;
                    }
                }
                // free(alpha);
            }
        }
    }
}

int remove_task_all_CS(int task_id, InterObject **Oj,int num){
    for(int i=0;i<num;i++){
        remove_CS_per_intersection_obj(Oj[i],task_id);
    }
    return 0;
}



// end...update algorithm


