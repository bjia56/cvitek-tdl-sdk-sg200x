#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
//#include "core.hpp"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"

#define EPOLL
#define HTTPSERVER_IMPL
#include "httpserver/httpserver.h"


cvitdl_handle_t tdl_handle = NULL;
imgprocess_t img_handle;
std::string str_src_dir = "/tmp/1.jpg";

void handle_request(struct http_request_s* request) {
  struct http_string_s body = http_request_body(request);
  struct http_response_s *response = http_response_init();

  std::ofstream out(str_src_dir, std::ios::trunc | std::ios::binary);
  out.write(body.buf, body.len);
  out.close();

  VIDEO_FRAME_INFO_S fdFrame;
  CVI_S32 ret = CVI_TDL_ReadImage(img_handle, str_src_dir.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888);
  if (ret != CVI_SUCCESS) {
    std::cout << "Convert out video frame failed with :" << ret << ".file:" << str_src_dir
              << std::endl;
    http_response_status(response, 500);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, "Convert out video frame failed", 30);
    http_respond(request, response);
    return;
  }

  cvtdl_object_t obj_meta = {0};

  CVI_TDL_Yolov5(tdl_handle, &fdFrame, &obj_meta);

  std::stringstream resp_body;
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    resp_body << obj_meta.info[i].bbox.x1 << " " << obj_meta.info[i].bbox.y1 << " "
              << obj_meta.info[i].bbox.x2 << " " << obj_meta.info[i].bbox.y2 << " "
              << obj_meta.info[i].bbox.score << " " << obj_meta.info[i].classes << std::endl;
  }

  http_response_status(response, 200);
  http_response_header(response, "Content-Type", "text/plain");
  http_response_body(response, resp_body.str().c_str(), resp_body.str().size());
  http_respond(request, response);

  CVI_TDL_ReleaseImage(img_handle, &fdFrame);
  CVI_TDL_Free(&obj_meta);
  //CVI_TDL_Destroy_ImageProcessor(img_handle);
}

int main(int argc, char *argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  std::string model_path = argv[1];
  int port = atoi(argv[2]);

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
  }

  // set thershold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.5);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.5);

  std::cout << "model opened:" << model_path << std::endl;

  CVI_TDL_Create_ImageProcessor(&img_handle);

  struct http_server_s *server = http_server_init(port, handle_request);
  return http_server_listen(server);
}