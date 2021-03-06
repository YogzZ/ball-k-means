﻿//This version is completed by Yong Zheng(413511280@qq.com), Shuyin Xia（380835019@qq.com）, Xingxin Chen, Junkuan Wang.  2020.5.1

#include <iostream>
#include <fstream>
#include <time.h>
#include <cstdlib>
#include <algorithm>
#include  <Eigen/Dense>
#include <vector>

using namespace std;
using namespace Eigen;

typedef float OurType;

typedef VectorXf VectorOur;

typedef MatrixXf MatrixOur;

typedef vector <vector<OurType>> ClusterDistVector;

typedef vector <vector<unsigned int>> ClusterIndexVector;

typedef Array<bool, 1, Dynamic> VectorXb;

typedef struct Neighbor
//Define the "neighbor" structure
{
    OurType distance;
    int index;
};

typedef vector <Neighbor> sortedNeighbors;

MatrixOur load_data(const char *filename);

inline MatrixOur
update_centroids(MatrixOur &dataset, ClusterIndexVector &cluster_point_index, unsigned int k, unsigned int n,
              VectorXb &flag,
              unsigned int iteration_counter, MatrixOur &old_centroids);


inline void update_radius(MatrixOur &dataset, ClusterIndexVector &cluster_point_index, MatrixOur &new_centroids,
                       ClusterDistVector &temp_dis,
                       VectorOur &the_rs, VectorXb &flag, unsigned int iteration_counter, unsigned int &dis_num,
                       unsigned int the_rs_size);

inline sortedNeighbors
get_sorted_neighbors(VectorOur &the_Rs, MatrixOur &centers_dis, unsigned int now_ball, unsigned int k,
                         vector<unsigned int> &now_center_index);

inline void
cal_centers_dist(MatrixOur &new_centroids, unsigned int iteration_counter, unsigned int k, VectorOur &the_rs,
                     VectorOur &delta, MatrixOur &centers_dis);

inline MatrixOur cal_dist(MatrixOur &dataset, MatrixOur &centroids);

inline MatrixOur
cal_ring_dist(unsigned j, unsigned int data_num, unsigned int dataset_cols, MatrixOur &dataset, MatrixOur &now_centers,
              ClusterIndexVector &now_data_index);

void initialize(MatrixOur &dataset, MatrixOur &centroids, VectorOur &labels, ClusterIndexVector &cluster_point_index,
                ClusterIndexVector &now_centers_index,
                ClusterDistVector &temp_dis);


void run(MatrixOur &dataset, MatrixOur &centroids)
{

    double start_time, end_time;


    bool judge = true;

    const unsigned int dataset_rows = dataset.rows();
    const unsigned int dataset_cols = dataset.cols();
    const unsigned int k = centroids.rows();

    ClusterIndexVector temp_cluster_point_index;
    ClusterIndexVector cluster_point_index;
    ClusterIndexVector now_centers_index;
    ClusterIndexVector now_data_index;
    ClusterDistVector temp_dis;

    MatrixOur new_centroids(k, dataset_cols);
    MatrixOur old_centroids = centroids;
    MatrixOur centers_dis(k, k);

    VectorXb flag(k);
    VectorXb old_flag(k);

    VectorOur labels(dataset_rows);
    VectorOur delta(k);

    vector<unsigned int> old_now_index;
    vector <OurType> distance_arr;

    VectorOur the_rs(k);

    unsigned int now_centers_rows;
    unsigned int iteration_counter;
    unsigned int num_of_neighbour;
    unsigned int neighbour_num;
    unsigned int dis_num;
    unsigned int data_num;

    MatrixOur::Index minCol;
    new_centroids.setZero();
    iteration_counter = 0;
    num_of_neighbour = 0;
    dis_num = 0;
    flag.setZero();

    //initialize cluster_point_index and temp_dis
    initialize(dataset, centroids, labels, cluster_point_index, now_centers_index, temp_dis);

    temp_cluster_point_index.assign(cluster_point_index.begin(), cluster_point_index.end());

    start_time = clock();

    while (true) {
        old_flag = flag;
        //record cluster_point_index from the previous round
        cluster_point_index.assign(temp_cluster_point_index.begin(), temp_cluster_point_index.end());
        iteration_counter += 1;


        //update the matrix of centroids
        new_centroids = update_centroids(dataset, cluster_point_index, k, dataset_cols, flag, iteration_counter,
                                      old_centroids);

        if (new_centroids != old_centroids) {
            //delta: distance between each center and the previous center
            delta = (((new_centroids - old_centroids).rowwise().squaredNorm())).array().sqrt();

            old_centroids = new_centroids;

            //get the radius of each centroids
            update_radius(dataset, cluster_point_index, new_centroids, temp_dis, the_rs, flag, iteration_counter, dis_num,
                       k);
            //Calculate distance between centers

            cal_centers_dist(new_centroids, iteration_counter, k, the_rs, delta, centers_dis);

            flag.setZero();

            //returns the set of neighbors

            //nowball;
            unsigned int now_num = 0;
            for (unsigned int now_ball = 0; now_ball < k; now_ball++) {
                sortedNeighbors neighbors = get_sorted_neighbors(the_rs, centers_dis, now_ball, k,
                                                                     now_centers_index[now_ball]);


                now_num = temp_dis[now_ball].size();
                if (the_rs(now_ball) == 0) continue;

                //Get the coordinates of the neighbors and neighbors of the current ball
                old_now_index.clear();
                old_now_index.assign(now_centers_index[now_ball].begin(), now_centers_index[now_ball].end());
                now_centers_index[now_ball].clear();
                neighbour_num = neighbors.size();
                MatrixOur now_centers(neighbour_num, dataset_cols);

                for (unsigned int i = 0; i < neighbour_num; i++) {
                    now_centers_index[now_ball].push_back(neighbors[i].index);
                    now_centers.row(i) = new_centroids.row(neighbors[i].index);

                }
                num_of_neighbour += neighbour_num;

                now_centers_rows = now_centers.rows();

                judge = true;

                if (now_centers_index[now_ball] != old_now_index)
                    judge = false;
                else {
                    for (int i = 0; i < now_centers_index[now_ball].size(); i++) {
                        if (old_flag(now_centers_index[now_ball][i]) != false) {
                            judge = false;
                            break;
                        }
                    }
                }

                if (judge) {
                    continue;
                }

                now_data_index.clear();
                distance_arr.clear();

                for (unsigned int j = 1; j < neighbour_num; j++) {
                    distance_arr.push_back(centers_dis(now_centers_index[now_ball][j], now_ball) / 2);
                    now_data_index.push_back(vector<unsigned int>());
                }

                for (unsigned int i = 0; i < now_num; i++) {
                    for (unsigned int j = 1; j < neighbour_num; j++) {
                        if (j == now_centers_rows - 1 && temp_dis[now_ball][i] > distance_arr[j - 1]) {
                            now_data_index[j - 1].push_back(cluster_point_index[now_ball][i]);
                            break;
                        }
                        if (j != now_centers_rows - 1 && temp_dis[now_ball][i] > distance_arr[j - 1] &&
                            temp_dis[now_ball][i] <= distance_arr[j]) {
                            now_data_index[j - 1].push_back(cluster_point_index[now_ball][i]);
                            break;
                        }
                    }
                }

                judge = false;
                int lenth = old_now_index.size();
                //Divide area
                for (unsigned int j = 1; j < neighbour_num; j++) {


                    data_num = now_data_index[j - 1].size();

                    if (data_num == 0)
                        continue;

                    MatrixOur temp_distance = cal_ring_dist(j, data_num, dataset_cols, dataset, now_centers,
                                                            now_data_index);
                    dis_num += data_num * j;

                    unsigned int new_label;
                    for (unsigned int i = 0; i < data_num; i++) {
                        temp_distance.row(i).minCoeff(&minCol);
                        new_label = now_centers_index[now_ball][minCol];
                        if (labels[now_data_index[j - 1][i]] != new_label) {
                            flag(now_ball) = true;
                            flag(new_label) = true;

                            //Update localand global labels
                            vector<unsigned int>::iterator it = (temp_cluster_point_index[labels[now_data_index[j -
                                                                                                                1][i]]]).begin();
                            while ((it) != (temp_cluster_point_index[labels[now_data_index[j - 1][i]]]).end()) {
                                if (*it == now_data_index[j - 1][i]) {
                                    it = (temp_cluster_point_index[labels[now_data_index[j - 1][i]]]).erase(it);
                                    break;
                                } else
                                    ++it;
                            }
                            temp_cluster_point_index[new_label].push_back(now_data_index[j - 1][i]);
                            labels[now_data_index[j - 1][i]] = new_label;
                        }
                    }
                }
            }
        } else
            break;
    }
    end_time = clock();
    cout << "iterations       :                  ||" << iteration_counter << endl;
    cout << "The number of calculating distance: ||" << cal_dist_num << endl;
    cout << "The number of neighbors:            ||" << num_of_neighbour << endl;
    cout << "Time per round：                    || " << (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000 / iteration_counter << endl;
}

MatrixOur load_data(const char *filename)
{
    /*

    *Summary: Read data through file path

    *Parameters:

    *     filename: file path.*    

    *Return : Dataset in eigen matrix format.

    */

    int x = 0, y = 0;  // x: rows  ，  y/x: cols
    ifstream inFile(filename, ios::in);
    string lineStr;
    while (getline(inFile, lineStr)) {
        stringstream ss(lineStr);
        string str;
        while (getline(ss, str, ','))
            y++;
        x++;
    }
    MatrixOur data(x, y / x);
    ifstream inFile2(filename, ios::in);
    string lineStr2;
    int i = 0;
    while (getline(inFile2, lineStr2)) {
        stringstream ss2(lineStr2);
        string str2;
        int j = 0;
        while (getline(ss2, str2, ',')) {
            data(i, j) = atof(const_cast<const char *>(str2.c_str()));
            j++;
        }
        i++;
    }
    return data;
}

inline MatrixOur
update_centroids(MatrixOur &dataset, ClusterIndexVector &cluster_point_index, unsigned int k, unsigned int n,
              VectorXb &flag, unsigned int iteration_counter, MatrixOur &old_centroids) {
    /*

    *Summary: Update the center point of each cluster

    *Parameters:

    *     dataset: dataset in eigen matrix format.*   

    *     clusters_point_index: global position of each point in the cluster.* 

    *     k: number of center points.*  

    *     dataset_cols: data set dimensions*  

    *     flag: judgment label for whether each cluster has changed.*  

    *     iteration_counter: number of iterations.*  

    *     old_centroids: center matrix of previous round.*  

    *Return : updated center matrix.

    */

    unsigned int cluster_point_index_size = 0;
    unsigned int temp_num = 0;
    MatrixOur new_c(k, n);
    VectorOur temp_array(n);
    for (unsigned int i = 0; i < k; i++) {
        temp_num = 0;
        temp_array.setZero();
        cluster_point_index_size = cluster_point_index[i].size();
        if (flag(i) != 0 || iteration_counter == 1) {
            for (unsigned int j = 0; j < cluster_point_index_size; j++) {
                temp_array += dataset.row(cluster_point_index[i][j]);
                temp_num++;
            }
            new_c.row(i) = temp_array / temp_num;
        } else new_c.row(i) = old_centroids.row(i);
    }
    return new_c;
}

inline void update_radius(MatrixOur &dataset, ClusterIndexVector &cluster_point_index, MatrixOur &new_centroids,
                       ClusterDistVector &temp_dis, VectorOur &the_rs, VectorXb &flag, unsigned int iteration_counter,
                       unsigned int &dis_num, unsigned int the_rs_size) {

    /*

    *Summary: Update the radius of each cluster

    *Parameters:

    *     dataset: dataset in eigen matrix format.*   

    *     clusters_point_index: global position of each point in the cluster.* 

    *     new_centroids: updated center matrix.*  

    *     point_center_dist: distance from point in cluster to center*  

    *     the_rs: The radius of each cluster.*  

    *     flag: judgment label for whether each cluster has changed.*  

    *     iteration_counter: number of iterations.*  

    *     cal_dist_num: distance calculation times.* 

    *     the_rs_size: number of clusters.* 

    */

    OurType temp = 0;
    unsigned int cluster_point_index_size = 0;
    for (unsigned int i = 0; i < the_rs_size; i++) {
        cluster_point_index_size = cluster_point_index[i].size();
        if (flag(i) != 0 || iteration_counter == 1) {
            the_rs(i) = 0;
            temp_dis[i].clear();
            for (unsigned int j = 0; j < cluster_point_index_size; j++) {
                dis_num++;
                temp = sqrt((new_centroids.row(i) - dataset.row(cluster_point_index[i][j])).squaredNorm());
                temp_dis[i].push_back(temp);
                if (the_rs(i) < temp) the_rs(i) = temp;
            }
        }
    }
};

bool LessSort(Neighbor a, Neighbor b) {
    return (a.distance < b.distance);
}

inline sortedNeighbors
get_sorted_neighbors(VectorOur &the_Rs, MatrixOur &centers_dis, unsigned int now_ball, unsigned int k,
                         vector<unsigned int> &now_center_index) {

    /*

    *Summary: Get the sorted neighbors

    *Parameters:

    *     the_rs: the radius of each cluster.*   

    *     centers_dist: distance matrix between centers.* 

    *     now_ball: current ball label.*  

    *     k: number of center points*  

    *     now_center_index: nearest neighbor label of the current ball.*  

    */

    VectorXi flag = VectorXi::Zero(k);
    sortedNeighbors neighbors;

    Neighbor temp;
    temp.distance = 0;
    temp.index = now_ball;
    neighbors.push_back(temp);
    flag(now_ball) = 1;


    for (unsigned int j = 1; j < now_center_index.size(); j++) {
        if (centers_dis(now_ball, now_center_index[j]) == 0 ||
            2 * the_Rs(now_ball) - centers_dis(now_ball, now_center_index[j]) < 0) {
            flag(now_center_index[j]) = 1;
        } else {
            flag(now_center_index[j]) = 1;
            temp.distance = centers_dis(now_ball, now_center_index[j]);
            temp.index = now_center_index[j];
            neighbors.push_back(temp);
        }
    }


    for (unsigned int j = 0; j < k; j++) {
        if (flag(j) == 1) {
            continue;
        }
        if (centers_dis(now_ball, j) != 0 && 2 * the_Rs(now_ball) - centers_dis(now_ball, j) >= 0) {
            temp.distance = centers_dis(now_ball, j);
            temp.index = j;
            neighbors.push_back(temp);
        }

    }

    sort(neighbors.begin(), neighbors.end(), LessSort);
    return neighbors;
}


inline void
cal_centers_dist(MatrixOur &new_centroids, unsigned int iteration_counter, unsigned int k, VectorOur &the_rs,
                     VectorOur &delta, MatrixOur &centers_dis) {

    /*

    *Summary: Calculate the distance matrix between center points

    *Parameters:

    *     new_centroids: current center matrix.*   

    *     iteration_counter: number of iterations.* 

    *     k: number of center points.*  

    *     the_rs: the radius of each cluster*  

    *     delta: distance between each center and the previous center.*  

    *     centers_dist: distance matrix between centers.*  

    */

    if (iteration_counter == 1) centers_dis = cal_dist(new_centroids, new_centroids).array().sqrt();
    else {
        for (unsigned int i = 0; i < k; i++) {
            for (unsigned int j = 0; j < k; j++) {
                if (centers_dis(i, j) >= 2 * the_rs(i) + delta(i) + delta(j))
                    centers_dis(i, j) = centers_dis(i, j) - delta(i) - delta(j);
                else {
                    centers_dis(i, j) = sqrt((new_centroids.row(i) - new_centroids.row(j)).squaredNorm());
                }
            }
        }
    }
}

inline MatrixOur cal_dist(MatrixOur &dataset, MatrixOur &centroids) {

    /*

    *Summary: Calculate distance matrix between dataset and center point

    *Parameters:

    *     dataset: dataset matrix.*   

    *     centroids: centroids matrix.* 

    *Return : distance matrix between dataset and center point.

    */

    return (((-2 * dataset * (centroids.transpose())).colwise() + dataset.rowwise().squaredNorm()).rowwise() +
            (centroids.rowwise().squaredNorm()).transpose());
}

inline MatrixOur
cal_ring_dist(unsigned j, unsigned int data_num, unsigned int dataset_cols, MatrixOur &dataset, MatrixOur &now_centers,
              ClusterIndexVector &now_data_index) {

    /*

    *Summary: Calculate the distance matrix from the point in the ring area to the corresponding nearest neighbor

    *Parameters:

    *     j: the label of the current ring.* 

    *     data_num: number of points in the ring area.*   

    *     dataset_cols: data set dimensions.* 

    *     dataset: dataset in eigen matrix format.* 

    *     now_centers: nearest ball center matrix corresponding to the current ball.* 

    *     now_data_index: labels for points in each ring.* 

    *Return : distance matrix from the point in the ring area to the corresponding nearest neighbor.

    */

    MatrixOur data_in_area(data_num, dataset_cols);

    for (unsigned int i = 0; i < data_num; i++) {
        data_in_area.row(i) = dataset.row(now_data_index[j - 1][i]);
    }

    Ref <MatrixOur> centers_to_cal(now_centers.topRows(j + 1));

    return (((-2 * data_in_area * (centers_to_cal.transpose())).colwise() +
             data_in_area.rowwise().squaredNorm()).rowwise() + (centers_to_cal.rowwise().squaredNorm()).transpose());
}

void initialize(MatrixOur &dataset, MatrixOur &centroids, VectorOur &labels, ClusterIndexVector &cluster_point_index,
                ClusterIndexVector &now_centers_index, ClusterDistVector &temp_dis) {

    /*

    *Summary: Initialize related variables

    *Parameters:

    *     dataset: dataset in eigen matrix format.*   

    *     centroids: dcentroids matrix.* 

    *     labels: the label of the cluster where each data is located.* 

    *     clusters_point_index: two-dimensional vector of data point labels within each cluster.* 

    *     clusters_neighbors_index: two-dimensional vector of neighbor cluster labels for each cluster.* 

    *     point_center_dist: distance from point in cluster to center.* 

    */

    MatrixOur::Index minCol;
    for (int i = 0; i < centroids.rows(); i++) {
        cluster_point_index.push_back(vector<unsigned int>());
        now_centers_index.push_back(vector<unsigned int>());
        temp_dis.push_back(vector<OurType>());
    }
    MatrixOur M = cal_dist(dataset, centroids);
    for (int i = 0; i < dataset.rows(); i++) {
        M.row(i).minCoeff(&minCol);
        labels(i) = minCol;
        cluster_point_index[minCol].push_back(i);
    }
}


int main(int argc, char *argv[]) {
    MatrixOur dataset = load_data("c:/users/yog/source/repos/test20/test20/data+centers/dataset/Codrna.csv") ;
    MatrixOur centroids = load_data("c:/users/yog/source/repos/test20/test20/data+centers/centroids/Codrna4.csv") ;
    run(dataset, centroids);

}