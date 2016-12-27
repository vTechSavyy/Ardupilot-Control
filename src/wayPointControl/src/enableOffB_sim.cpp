#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>

mavros_msgs::State current_state;
void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
}

geometry_msgs::PoseStamped targetPose;
void get_Pos(const geometry_msgs::PoseStamped::ConstPtr& msg2){
  targetPose=*msg2;
}

int main(int argc, char **argv)
{
/*
double xSetPos;
double ySetPos;
double zSetPos;


   //Parse arguments:

   if (argc>0){
     xSetPos= *argv[0];
     ySetPos= *argv[1];
     zSetPos= *argv[2];

   }
*/
    ros::init(argc, argv, "offb_node_sim");
    ros::NodeHandle nh;

    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>
            ("mavros/state", 10, state_cb);

    ros::Subscriber pose_sub= nh.subscribe("savio/Pose",10,get_Pos);

    ros::Publisher local_pos_pub = nh.advertise<geometry_msgs::PoseStamped>
            ("mavros/setpoint_position/local", 10);

    ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool>
            ("mavros/cmd/arming");

    ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>
            ("mavros/set_mode");

    //the setpoint publishing rate MUST be faster than 2Hz
    ros::Rate rate(20.0);

    // wait for FCU connection
    while(ros::ok() && current_state.connected){
        ros::spinOnce();
        rate.sleep();
    }

    //geometry_msgs::PoseStamped pose;
    //pose.pose.position.x = xSetPos;
    //pose.pose.position.y = ySetPos;
    //pose.pose.position.z = zSetPos;


    //send a few setpoints before starting
    for(int i = 100; ros::ok() && i > 0; --i){
        local_pos_pub.publish(targetPose);
        ros::spinOnce();
        rate.sleep();
    }



    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.custom_mode = "OFFBOARD";
    //offb_set_mode.request.custom_mode = "OFFBOARD";

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;

    ros::Time last_request = ros::Time::now();

    // Removing the OFFBOARD mode command from the loop:

        // Check whether the current mode is OFFBOARD
        if( current_state.mode != "OFFBOARD") { //&&
            //(ros::Time::now() - last_request > ros::Duration(15.0))){

            if( set_mode_client.call(offb_set_mode) &&
                offb_set_mode.response.success){
                ROS_INFO("OFFBOARD mode enabled");
            }

            last_request = ros::Time::now();


        }
        // If the current is mode is OFFBOARD then arm the vehicle:
        while(ros::ok()){
            if( current_state.mode=="OFFBOARD" && !current_state.armed &&
                (ros::Time::now() - last_request > ros::Duration(15.0))){
                if( arming_client.call(arm_cmd) &&
                    arm_cmd.response.success){
                    ROS_INFO("Vehicle armed");
                }
                last_request = ros::Time::now();
            }




        local_pos_pub.publish(targetPose);

        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}
