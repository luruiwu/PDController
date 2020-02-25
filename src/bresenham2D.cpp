

#include "bresenham2D.h"
#include "costmap_2d/costmap_2d_ros.h"
#include <random>
#include <thread>
#include <future>


bresenham2D::bresenham2D()
{    
  refresh_data();
  costmap_subscriber = nh_.subscribe("/move_base/local_costmap/costmap",1,&bresenham2D::costmapCallback,this);  
  amcl_pose = nh_.subscribe("/amcl_pose", 1 , &bresenham2D::amcl_callback, this);

  //controller_costmap_ros_ = controller_costmap_ros_;
}

int bresenham2D::compute(int *x)
{

    if (m.try_lock())
    {
    
    find_line(curr_pose_x,curr_pose_y,goalX,goalY);

    if (dot_product())
    {
        ROS_ERROR("I amhere. dot product grteater than one");
        *x = 1;
        //refresh_data();
        m.unlock();
        
        return 0;
    }

    
    }

    else{
            ROS_ERROR("lock not recieved");
        }

    m.unlock();

    *x = 0;


}



void bresenham2D::amcl_callback(const geometry_msgs::PoseWithCovarianceStampedConstPtr pose)
{
    
    curr_pose_x = pose->pose.pose.position.x;
    curr_pose_y = pose->pose.pose.position.y;

}

bool bresenham2D::check_robot_path(double goalX, double goalY)
{
    /*
    true: there's an obstacle or cannot be sent

    */
    //footprintCallback();

     	
    // Create a thread using member function
    //std::thread th(&Task::execute, taskPtr, "Sample Task");


    if ((!(*costmap_)) ){
        ROS_INFO("Either there's no footprint or costmap here!");
        ros::Duration(1).sleep();
        return true;}

    std::thread t1(&bresenham2D::compute, this, &sem) ;
    t1.join();


    ROS_ERROR("sem is %d", sem);
    if (sem == 1)
    {
        ROS_ERROR("bad path");
        return true;
    }

    
    
    //points_inside_ellipse(curr_pose_x, curr_pose_y);

    

    return false;


}

void bresenham2D::test(double goalX, double goalY)
{
    find_line(curr_pose_x,curr_pose_y,goalX,goalY);
}

void bresenham2D::find_line(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2-y1;
    double P = 2*dy - dx;

    double x = x1;
    double y = y1;

    while (x <= x2){
        if (*kill_p)
        {
            return;
        }
        //put_point(x , y);
        points_inside_ellipse(x , y);

        x += *resolution;

        if (P < 0){ P = P +2 * dy;}

        else{
            P = P +2 * dy - 2*dx;
            y += *resolution;
        }
    }
}

void bresenham2D::points_inside_ellipse( double x_0 , double y_0)
{

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<> rho(0 , 1);
    std::uniform_real_distribution<> phi(0 , PI);

    


    for (int  i = 0 ; i < 1000 ; i++)
    {
        double x = sqrt(rho(rng)) * cos(phi(rng));
        double y = sqrt(rho(rng)) * sin(phi(rng));


        x = x_0 + x * 1.5;
        y = y_0 + y * 1.5;

        put_point(x,y);

    }

    *semaphore = true;



}



void bresenham2D::put_point(double x ,double y)
{


    //ROS_INFO("origin x and y %f %f  ", *originX , *originY);
    int columns = (x- *originX)/ *resolution;
    int rows = (y- *originY)/ *resolution;

    //ROS_INFO("origin x and y are %f %f", *originX , *originY);

    //ROS_INFO("x and y are %f %f", x , y);

    int index = rows * 80 + columns;

    //ROS_INFO("%d Index", index);

    if (index < 0)
    {
        return;
    }

    //print_corresponding_coordinates(index);
    if (index < (width*height))
    {
            
         data[index] = 100;
    }

    

    else{
        kill = true;
    }
    

}

void bresenham2D::refresh_data()
{
    pd_controller::ray rays = pd_controller::ray();
    for (auto i = 0 ; i< (80 * 80) ; i++)
    {
                                                                                                                                                                                                                                             
        data[i] = 0;
    }
}




bool bresenham2D::dot_product()
{
    double c = 0;
    for (auto i = 0; i< (width*height); i++)
    {
        //ROS_INFO("%d %d", data[i], costmap_data[i]);
        c += data[i]*costmap_data[i];
    }

    if (c > 0 ){
        
    ROS_INFO("dot product %f" , c);
    return true;
    }

    return false;
}


void bresenham2D::costmapCallback(const nav_msgs::OccupancyGrid::ConstPtr& costmap )
{

        if (m.try_lock())
        {


        bool flag = false;
        if (*costmap_count_ == 0)
        {
            
            flag = true;
            prev = ros::Time::now().toSec();
        }

        ROS_INFO("Unsuccesful duration %f", (ros::Time::now().toSec() - prev ) );

        (*costmap_count_)+=1;

        if (true){
            ROS_INFO("Got costmap");
            height = costmap->info.height;
            width = costmap->info.width;
            resolution_ = costmap->info.resolution;
            
            originX_ = costmap->info.origin.position.x;
            
            originY_ = costmap->info.origin.position.y;
            
            *costmap_ = true;

            prev = ros::Time::now().toSec();
        

            for (auto i = 0 ; i< (height*width) ; i++)
            {
                costmap_data[i] = costmap->data[i];
                    //ROS_INFO("%d", i);

                if (costmap->data[i] > 50){
                        //ROS_INFO("obstacle at %d", i);
                    //    //print_corresponding_coordinates(i);
                    //}
                }
            }

        }
        m.unlock();
        }


        else{
            ROS_ERROR("lock not recieved");
        }

    //ROS_INFO("costmap origin x and y %f %f  ", *originX , *originY);
        

}

    
    
    void bresenham2D::print_corresponding_coordinates(int x)
    {
        
        int rows = x / 80 ;
        int columns = x % 80;

        double newX = *originX + *resolution*(columns);
        double newY = *originY + *resolution* (rows);
        ROS_INFO("%f %f", newX, newY);
    }

    /*
    
int main (int argc, char** argv){
    ros::init(argc,argv,"bresenham");
    ros::NodeHandle nh;
    ros::Subscriber goal;
    
    
    bresenham2D b;
    ros::spin();
}
*/
