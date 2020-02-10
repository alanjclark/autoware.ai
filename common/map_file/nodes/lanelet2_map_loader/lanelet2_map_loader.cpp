/*
 * Copyright 2019 Autoware Foundation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authors: Simon Thompson, Ryohsuke Mitsudome
 *
 */

#include <ros/ros.h>

#include <lanelet2_projection/UTM.h>
#include <lanelet2_io/Io.h>
#include <lanelet2_core/LaneletMap.h>

#include <lanelet2_extension/utility/message_conversion.h>
#include <lanelet2_extension/projection/mgrs_projector.h>
#include <lanelet2_extension/projection/local_frame_projector.h>
#include <lanelet2_extension/io/autoware_osm_parser.h>
#include <lanelet2_extension/utility/utilities.h>

#include <autoware_lanelet2_msgs/MapBin.h>

#include <string>

void printUsage()
{
  ROS_ERROR_STREAM("Usage:");
  ROS_ERROR_STREAM("rosrun map_file lanelet2_map_loader [.OSM]");
  ROS_ERROR_STREAM("rosrun map_file lanelet2_map_loader download [X] [Y]: WARNING not implemented");
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "lanelet_map_loader");
  ros::NodeHandle rosnode;
  ros::NodeHandle private_nh("~");

  if (argc < 2)
  {
    printUsage();
    return EXIT_FAILURE;
  }

  std::string mode(argv[1]);
  if (mode == "download" && argc < 4)
  {
    printUsage();
    return EXIT_FAILURE;
  }

  std::string lanelet2_filename(argv[1]);

  lanelet::ErrorMessages errors;
  lanelet::LaneletMapPtr map;

  int projector_type = 0;
  private_nh.param<int>("projector_type", projector_type, 1);
  if(projector_type == 0)
  {
    lanelet::projection::MGRSProjector projector;
    map = lanelet::load(lanelet2_filename, projector, &errors);
  } else if(projector_type == 1)
  {
    std::string base_frame , target_frame;
    private_nh.param<std::string>("base_frame", base_frame, "EPSG:4326");
    private_nh.param<std::string>("target_frame", target_frame, "");
    lanelet::projection::LocalFrameProjector local_projector(base_frame.c_str(), target_frame.c_str());
    map = lanelet::load(lanelet2_filename, local_projector, &errors);
  }

  for(const auto &error: errors)
  {
    ROS_ERROR_STREAM(error);
  }
  if(!errors.empty())
  {
    return EXIT_FAILURE;
  }

  lanelet::utils::overwriteLaneletsCenterline(map, false);

  std::string format_version, map_version;
  lanelet::io_handlers::AutowareOsmParser::parseVersions(lanelet2_filename, &format_version, &map_version);

  ros::Publisher map_bin_pub = rosnode.advertise<autoware_lanelet2_msgs::MapBin>("/lanelet_map_bin", 1, true);
  autoware_lanelet2_msgs::MapBin map_bin_msg;
  map_bin_msg.header.stamp = ros::Time::now();
  map_bin_msg.header.frame_id = "map";
  map_bin_msg.format_version = format_version;
  map_bin_msg.map_version = map_version;
  lanelet::utils::conversion::toBinMsg(map, &map_bin_msg);

  map_bin_pub.publish(map_bin_msg);

  ros::spin();

  return 0;
}