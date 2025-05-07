#include "object.h"
#include <arpa/inet.h>

int main(){
    // 1.all traffic
    InterObject *obj0 = (InterObject *)malloc(sizeof(InterObject));
    obj0->direction = 1;
    obj0->src_ip = 0;
    obj0->src_mask = 0;
    obj0->dst_ip = 0;
    obj0->dst_mask = 0;
    obj0->src_port = 0;
    obj0->dst_port = 0;
    obj0->protocol = 0;
    obj0->priority = 0;
    obj0->parrent_obj_id = -1;
    obj0->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj0->hosts[i] = 0;
    }

    // 2.逐渐缩小
    InterObject *obj110 = (InterObject *)malloc(sizeof(InterObject));
    obj110->direction = 0;
    obj110->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj110->src_mask = ntohl(inet_addr("255.255.255.128"));
    obj110->dst_ip = 0;
    obj110->dst_mask = 0;
    obj110->src_port = 0;
    obj110->dst_port = 0;
    obj110->protocol = 0;
    obj110->priority = 0;
    obj110->parrent_obj_id = -1;
    obj110->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj110->hosts[i] = 0;
    }
   
    InterObject *obj111 = (InterObject *)malloc(sizeof(InterObject));
    obj111->direction = 0;
    obj111->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj111->src_mask = ntohl(inet_addr("255.255.255.128"));
    obj111->dst_ip = 0;
    obj111->dst_mask = 0;
    obj111->src_port = 0;
    obj111->dst_port = 0;
    obj111->protocol = 0;
    obj111->priority = 0;
    obj111->parrent_obj_id = -1;
    obj111->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj111->hosts[i] = 0;
    }

    InterObject *obj120 = (InterObject *)malloc(sizeof(InterObject));
    obj120->direction = 0;
    obj120->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj120->src_mask = ntohl(inet_addr("255.255.255.192"));
    obj120->dst_ip = 0;
    obj120->dst_mask = 0;
    obj120->src_port = 0;
    obj120->dst_port = 0;
    obj120->protocol = 0;
    obj120->priority = 0;
    obj120->parrent_obj_id = -1;
    obj120->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj120->hosts[i] = 0;
    }

    InterObject *obj121 = (InterObject *)malloc(sizeof(InterObject));
    obj121->direction = 0;
    obj121->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj121->src_mask = ntohl(inet_addr("255.255.255.192"));
    obj121->dst_ip = 0;
    obj121->dst_mask = 0;
    obj121->src_port = 0;
    obj121->dst_port = 0;
    obj121->protocol = 0;
    obj121->priority = 0;
    obj121->parrent_obj_id = -1;
    obj121->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj121->hosts[i] = 0;
    }

    InterObject *obj130 = (InterObject *)malloc(sizeof(InterObject));
    obj130->direction = 0;
    obj130->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj130->src_mask = ntohl(inet_addr("255.255.255.224"));
    obj130->dst_ip = 0;
    obj130->dst_mask = 0;
    obj130->src_port = 0;
    obj130->dst_port = 0;
    obj130->protocol = 0;
    obj130->priority = 0;
    obj130->parrent_obj_id = -1;
    obj130->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj130->hosts[i] = 0;
    }

    InterObject *obj131 = (InterObject *)malloc(sizeof(InterObject));
    obj131->direction = 0;
    obj131->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj131->src_mask = ntohl(inet_addr("255.255.255.224"));
    obj131->dst_ip = 0;
    obj131->dst_mask = 0;
    obj131->src_port = 0;
    obj131->dst_port = 0;
    obj131->protocol = 0;
    obj131->priority = 0;
    obj131->parrent_obj_id = -1;
    obj131->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj131->hosts[i] = 0;
    }

    InterObject *obj140 = (InterObject *)malloc(sizeof(InterObject));
    obj140->direction = 0;
    obj140->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj140->src_mask = ntohl(inet_addr("255.255.255.240"));
    obj140->dst_ip = 0;
    obj140->dst_mask = 0;
    obj140->src_port = 0;
    obj140->dst_port = 0;
    obj140->protocol = 0;
    obj140->priority = 0;
    obj140->parrent_obj_id = -1;
    obj140->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj140->hosts[i] = 0;
    }

    InterObject *obj141 = (InterObject *)malloc(sizeof(InterObject));
    obj141->direction = 1;
    obj141->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj141->src_mask = ntohl(inet_addr("255.255.255.240"));
    obj141->dst_ip = 0;
    obj141->dst_mask = 0;
    obj141->src_port = 0;
    obj141->dst_port = 0;
    obj141->protocol = 0;
    obj141->priority = 0;
    obj141->parrent_obj_id = -1;
    obj141->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj141->hosts[i] = 0;
    }

    
    InterObject *obj22 = (InterObject *)malloc(sizeof(InterObject));
    obj22->direction = 1;
    obj22->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj22->src_mask = ntohl(inet_addr("255.255.255.192"));
    obj22->dst_ip = 0;
    obj22->dst_mask = 0;
    obj22->src_port = 0;
    obj22->dst_port = 0;
    obj22->protocol = 0;
    obj22->priority = 0;
    obj22->parrent_obj_id = -1;
    obj22->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj22->hosts[i] = 0;
    }


    InterObject *obj23 = (InterObject *)malloc(sizeof(InterObject));
    obj23->direction = 0;
    obj23->src_ip = ntohl(inet_addr("1.0.0.0"));
    obj23->src_mask = ntohl(inet_addr("255.255.255.0"));
    obj23->dst_ip = ntohl(inet_addr("0.0.0.0"));
    obj23->dst_mask = ntohl(inet_addr("192.0.0.0"));
    obj23->src_port = 0;
    obj23->dst_port = 0;
    obj23->protocol = 0;
    obj23->priority = 0;
    obj23->parrent_obj_id = -1;
    obj23->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj23->hosts[i] = 0;
    }

    InterObject *obj24 = (InterObject *)malloc(sizeof(InterObject));
    obj24->direction = 0;
    obj24->src_ip = ntohl(inet_addr("1.0.0.0"));
    obj24->src_mask = ntohl(inet_addr("255.255.255.192"));
    obj24->dst_ip = ntohl(inet_addr("0.0.0.0"));
    obj24->dst_mask = ntohl(inet_addr("128.0.0.0"));
    obj24->src_port = 0;
    obj24->dst_port = 0;
    obj24->protocol = 0;
    obj24->priority = 0;
    obj24->parrent_obj_id = -1;
    obj24->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj24->hosts[i] = 0;
    }

    InterObject *obj3 = (InterObject *)malloc(sizeof(InterObject));
    if(get_intersection_object(obj0, obj22, obj3)){
        printf("obj0&obj22: ");
        print_intersection_object(obj3);
    }else{
        printf("no intersection\n");
    }
    if(get_intersection_object(obj22, obj23, obj3)){
        printf("obj22&obj23: ");
        print_intersection_object(obj3);
    }else{
        printf("no intersection\n");
    }
    if(get_intersection_object(obj23, obj24, obj3)){
        printf("obj23&obj24: ");
        print_intersection_object(obj3);
    }else{
        printf("no intersection\n");
    }
    if(get_intersection_object(obj22, obj24, obj3)){
        printf("obj22&obj24: ");
        print_intersection_object(obj3);
    }else{
        printf("no intersection\n");
    }
    init_objects();
    update_CS_per_intersection_obj(obj0, 0);
    print_s();
    update_CS_per_intersection_obj(obj0, 1);
    print_s();
    update_CS_per_intersection_obj(obj22, 2);
    print_s();
    update_CS_per_intersection_obj(obj23, 3);
    print_s();
    update_CS_per_intersection_obj(obj24, 4);
    print_s();
    remove_CS_per_intersection_obj(obj23, 3);
    print_s();
    remove_CS_per_intersection_obj(obj24, 4);
    print_s();
    remove_CS_per_intersection_obj(obj22, 2);
    print_s();
    remove_CS_per_intersection_obj(obj0, 1);
    print_s();
    remove_CS_per_intersection_obj(obj0, 0);
    print_s();
}