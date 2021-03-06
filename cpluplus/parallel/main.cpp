/*
 * main.cpp
 *
 *  Created on: Apr 15, 2021
 *      Author: ubuntu
 */
#include<iostream>
#include<fstream>
#include<cmath>
#include<assert.h>
#include<stdio.h>
#include<cmath>
#include<sys/time.h>
#include <string>
#include <sstream>
#include "mpi.h"
#include <omp.h>
#include "Serial_CGM.h"
#include "Parallel_CGM.h"
using namespace std;
#define _NUM_THREADS 4

const string file_name = "naive_fe_ddm";
float_t Hmax;

const int max_asmiter_num = 100;
int Num_Subdomain;
const float_t electric_charge = 1;
const float_t epsilon = 1;


//
int WriteOutput(vector<float_t>& phi){
	stringstream buf0, buf1;
	string hmax, num_sub;
	buf0 << Num_Subdomain;
	num_sub = buf0.str();
	buf1 << Hmax;
	hmax = buf1.str();
	string whole_filename = file_name + num_sub + "_out"  + "_" + hmax + ".txt";

	ofstream outfile(whole_filename,fstream::out);
	
	if (outfile.is_open())
	{
		for(auto a:phi) outfile << a << " ";
		outfile.close();
		cout << "output file written to " << whole_filename << endl;
		return 0;
	}
	else{
		cout << "Write Output txt file failed" << endl;
		return 1;
	}
}

int ReadSubdomainInfo(int subdomainIdx, int_t* Total_Num_Nodes, int_t* Num_Nodes,int_t* Num_Elements,vector<float_t>* coords,
		vector<int_t>* Element2NodeList, vector<int_t>* Nodes_Interior, vector<int_t>* Nodes_Interior2Global, vector<vector<int_t>>* Nodes_Interface,
		vector<int_t>* Node_Source, vector<int_t>* Elements_Node_Source, vector<int_t>* Nodes_Dirichlet,vector<vector<int_t>>* Nodes_othersABC,
		vector<int_t>* Nodes_Interface_ToWhichSubDomain, vector<int_t>* Nodes_Interface_FromWhichSubDomain){

	try{
		// subdomain geometry information generated by matlab
		stringstream buf0,buf1,buf2;
		string hmax,rank,num_sub;
		buf0<<Num_Subdomain;
		num_sub = buf0.str();
		buf1<<Hmax;
		hmax = buf1.str();
		buf2<<(subdomainIdx+1);
		rank = buf2.str();
		string whole_filename = file_name+num_sub+"_rank"+rank+"_"+hmax+".txt";
		ifstream infile(whole_filename,fstream::in); // load file
		if (!infile.is_open())
		{
			cout << "read " << whole_filename << " failed." << endl;
			return 0;
		}
		string one_line; // one line in the .txt file
		int_t node_id, element_id;  // temp variable
		float_t coord; // temp variable
		vector<int_t> lines; // temp variable
		vector<vector<int_t>> lines_feat; // temp variable

		getline(infile, one_line);
		stringstream stringin(one_line);
		stringin >> Total_Num_Nodes[0];

		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		stringin >> Num_Nodes[0];
		stringin >> Num_Elements[0];

		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		while (stringin >> coord) {
			coords->push_back(coord);
		}

		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		while (stringin >> node_id) {
			Element2NodeList->push_back(node_id);
		}

		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		while (stringin >> node_id) {
			Nodes_Interior->push_back(node_id);
		}

		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		while (stringin >> node_id) {
			Nodes_Interior2Global->push_back(node_id);
		}

		for(int j=0;j<Num_Subdomain;j++){
			getline(infile, one_line);
			stringin.clear();
			stringin.str(one_line);
			lines.clear();
			while (stringin >> node_id) {
				lines.push_back(node_id);
			}
			Nodes_Interface->push_back(lines);
		}

		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		while (stringin >> node_id) {
			Node_Source->push_back(node_id);
		}

		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		while (stringin >> element_id) {
			Elements_Node_Source->push_back(element_id);
		}



		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		while (stringin >> node_id) {
			Nodes_Dirichlet->push_back(node_id);
		}

		for(int i=0;i<Num_Subdomain;i++){
			getline(infile, one_line);
			stringin.clear();
			stringin.str(one_line);
			lines.clear();
			while (stringin >> node_id) {
				lines.push_back(node_id);
			}
			Nodes_othersABC->push_back(lines);
		}

		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		while (stringin >> node_id) {
			Nodes_Interface_ToWhichSubDomain->push_back(node_id);
		}

		getline(infile, one_line);
		stringin.clear();
		stringin.str(one_line);
		while (stringin >> node_id) {
			Nodes_Interface_FromWhichSubDomain->push_back(node_id);
		}

		getline(infile, one_line);

		if(!infile.eof()) cout << "... read sequence has some problem ..." << flush;
		else cout << "... read sequence has no problem ..." << flush;

		infile.close();
		return true;
	}
	catch (std::exception& e) {
		cout << e.what() << endl;
		return false;
	}

}

// select sub matrix of original matrix
vector<vector<float_t>> SubMatrix(vector<vector<float_t>>&A, vector<int_t>&rows,vector<int_t>&columns){
	vector<vector<float_t>> Asub;
	vector<float_t> Arow;
	
	if(columns[0]==-1){
		Arow.resize(columns.size());
		Arow.assign(columns.size(),0);
		for(auto row:rows){
			Asub.push_back(Arow);
		}
	}
	else{
		for(auto row:rows){
			Arow.clear();
			for(auto column:columns){
				Arow.push_back(A[row][column]);
			}
			Asub.push_back(Arow);
		}
	}

	return Asub;
}

// select sub array of original array
vector<float_t> SubArray(vector<float_t>&x, vector<int_t>&indices){
	vector<float_t> xsub;
	if(indices[0]==-1){
		xsub.resize(1);
		xsub.assign(1,0);
	}
	else{
		for(auto index:indices){
			xsub.push_back(x[index]);
		}
	}

	return xsub;
}

// FEM assembly
bool Assemble(vector<vector<float_t>>* K, vector<float_t>* b, const int_t Num_Nodes,const int_t Num_Elements,const vector<int_t> Element2NodeList,const vector<float_t> coords, const vector<int_t> Nodes_Dirichlet, const vector<int_t> Elements_Node_Source, const vector<int_t> Node_Source, const vector<float_t> eps){
	cout << "begin assemble " << " ... "<< flush;
	try{
		float_t ie[3], xe[3], ye[3], be[3], ce[3], Delta_e;
		vector<vector<float_t>> Ke(3,vector<float_t>(3,0));
		for(int_t e=0;e<Num_Elements;e++){
			for(int ne = 0; ne < 3; ++ne) {
				ie[ne] = Element2NodeList [3 * e + ne];
				xe[ne] = coords[2 * Element2NodeList [3 * e + ne]];
				ye[ne] = coords[2 * Element2NodeList [3 * e + ne] + 1];
			}
			be[0]=ye[1]-ye[2];
			be[1]=ye[2]-ye[0];
			be[2]=ye[0]-ye[1];

			ce[0]=xe[2]-xe[1];
			ce[1]=xe[0]-xe[2];
			ce[2]=xe[1]-xe[0];
			Delta_e=(be[0]*ce[1]-be[1]*ce[0])/2;
			for(int i=0;i<3;i++){
				for(int j=0;j<3;j++){
					Ke[i][j]=eps[e]/(4*Delta_e)*(be[i]*be[j]+ce[i]*ce[j]);
				}
			}
			for(int i=0;i<3;i++){
				for(int j=0;j<3;j++){
					(*K)[ie[i]][ie[j]]+=Ke[i][j];
				}
			}
		}
		cout << "K" << " ... "<< flush;
		// Impose the Dirichlet boundary condition
		int_t node_id, element_id;
		for(int i =0;i<Nodes_Dirichlet.size();i++){
			node_id = Nodes_Dirichlet[i];
			(*b)[Nodes_Dirichlet[i]]=0;
			(*K)[node_id][node_id]=1;
			for(int n=0;n<Num_Nodes;n++){
				if(n==node_id) continue;
				(*b)[n]-=(*K)[n][node_id]*0;
				(*K)[n][node_id]=0;
				(*K)[node_id][n]=0;
			}
		}
		cout << "Dirichlet" << " ... "<< flush;
		// Impose Source
		for(int e =0;e<Elements_Node_Source.size();e++){
			element_id = Elements_Node_Source[e];
			for(int ne = 0; ne < 3; ++ne) {
				ie[ne] = Element2NodeList [3 * element_id + ne];
				xe[ne] = coords[2 * Element2NodeList [3 * element_id + ne]];
				ye[ne] = coords[2 * Element2NodeList [3 * element_id + ne] + 1];
			}

			be[0]=ye[1]-ye[2];
			be[1]=ye[2]-ye[0];
			be[2]=ye[0]-ye[1];

			ce[0]=xe[2]-xe[1];
			ce[1]=xe[0]-xe[2];
			ce[2]=xe[1]-xe[0];
			Delta_e=(be[0]*ce[1]-be[1]*ce[0])/2;

			for(int n=0;n<Node_Source.size();n++){
				node_id=Node_Source[n];
				if(ie[0]==node_id||ie[1]==node_id||ie[2]==node_id)
					(*b)[node_id]+=electric_charge*1/3*Delta_e;
			}
		}
		cout << "Source" << " ... " << flush;
		return true;
	}
	catch(std::exception& e) {
		cout << e.what() << endl;
		return false;
	}
}

// assign values of sub array to original array
void AssignSubArray(vector<float_t>*x, vector<float_t>&xsub, vector<int_t>&indices){
	int_t i= 0;
	for(auto index:indices){
		x->at(index) = xsub[i++];
	}
}

int off_main(int argc,char **argv){
	// terms
	// rank --> one PE, responsible for one sub-domain
	// node --> a point with index and coordinate (x,y) where the finite element coefficient resides
	// AB(C) --> artificial boundary (condition)
	// DB(C) --> dirichlet boundary (condition)
	int myrank, nprocs;
	omp_set_num_threads(_NUM_THREADS);
	//MPI_Init(&argc, &argv);
	int provided;
	MPI_Init_thread( &argc, &argv, MPI_THREAD_FUNNELED, &provided);
	if(provided < MPI_THREAD_FUNNELED)
    {
        printf("The threading support level is lesser than that demanded.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    else
    {
        printf("The threading support level corresponds to that demanded.\n");
    }

	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	Num_Subdomain = nprocs;
	MPI_Request* req = (MPI_Request*)malloc(2*Num_Subdomain*sizeof(MPI_Request));

	string s  = argv[1];
	Hmax = stof(s);
	// timing
	timeval  start,end;
	// get the geometry information of each rank
	int_t Total_Num_Nodes,Num_Nodes,Num_Elements;
	vector<float_t> coords; // (x,y) for each node
	vector<int_t> Element2NodeList; // (n1,n2,n3) for triangular element (the elements solely inside each rank)
	vector<int_t> Node_Source; // the nodes where source resides, two for each rank in our toy problem
	vector<int_t> Elements_Node_Source; // the elements that contain the nodes in Node_Source
	vector<int_t> Nodes_Interior; // 1d
	vector<int_t> Nodes_Interior2Global;
	vector<int_t> Nodes_Interface_ToWhichSubDomain;
	vector<int_t> Nodes_Interface_FromWhichSubDomain;
	vector<vector<int_t>> Nodes_Interface; // 2d, 3*? , those on the AB && the ABCs enforced by the other three ranks.
	//vector<int_t> Nodes_Ghost; // 1d, those adjacent && outside the AB, but the elements they belong to will contribute to the AB entries in [K]
	vector<int_t> Nodes_Dirichlet; // 1d, the nodes on DB
	vector<vector<int_t>> Nodes_othersABC;

	if(ReadSubdomainInfo(myrank,&Total_Num_Nodes,&Num_Nodes,&Num_Elements, &coords, &Element2NodeList, &Nodes_Interior, &Nodes_Interior2Global,
			&Nodes_Interface, &Node_Source, &Elements_Node_Source, &Nodes_Dirichlet,&Nodes_othersABC, &Nodes_Interface_ToWhichSubDomain, &Nodes_Interface_FromWhichSubDomain)){
		cout << "rank "<< myrank <<" read sub-domain info success ...";
		cout << "Num_Nodes " << Num_Nodes << " ..."<< flush;
		cout << "Num_Elements " << Num_Elements << " ..."<< flush;
		cout << "Nodes_Interface_ToWhichSubDomain " << Nodes_Interface_ToWhichSubDomain.size() << " ..."<< flush;
		cout << "Nodes_Interface_FromWhichSubDomain " << Nodes_Interface_FromWhichSubDomain.size() << " ..."<< flush;
	}
	else{
		cout << "rank "<< myrank <<" read sub-domain info failed." << endl;
		return -1;
	}

	// assemble the sub system in each rank
	vector<vector<float_t>> K(Num_Nodes,vector<float_t>(Num_Nodes,0)); // the total number of nodes of the subdomain including those on the interface
	vector<float_t> b(Num_Nodes,0);
	vector<float_t> eps(Num_Elements,epsilon);
	vector<float_t> phi(Num_Nodes,0); // solution for the entire domain

	if (Assemble(&K, &b, Num_Nodes, Num_Elements, Element2NodeList, coords, Nodes_Dirichlet, Elements_Node_Source, Node_Source, eps))
	{
		cout << "rank " << myrank <<" assemble sub-domain system success ..."<< flush;
	}
	else{
		cout << "rank " << myrank <<" assemble sub-domain system failed." << endl;
		return -1;
	}

	// in MPI each node (rank) is responsible for one sub domain
	// the node index is global node index, its array index is the local node index (!)
	// send the phi value to the rank where it acts artificial boundary --> we have already know the index to send
	// node list include three kinds of nodes: interior nodes, interface nodes

	vector<float_t> b_in;
	vector<float_t> phi_in;
	vector<vector<float_t>> K_in;
	
	vector<vector<vector<float_t>>> KinABC(Num_Subdomain);
	// initialize send: select from the subdomain local phi array, in each iteration, we send the newly solved ones
	vector<vector<float_t>> buf_send(Num_Subdomain);
	vector<vector<float_t>> phiAB(Num_Subdomain);

	cout << "malloc success ..." << flush;
	vector<vector<float_t>> KinABC_temp;
	for(int i=0;i<Num_Subdomain;i++){
		//cout << "Rank "<< myrank << " from Subdomain " << Nodes_Interface_FromWhichSubDomain[i] << " of a size " << Nodes_Interface[i].size() << endl << flush;
		phiAB[i].assign(Nodes_Interface[i].size(),0);
		KinABC[i] = SubMatrix(K,Nodes_Interior,Nodes_Interface[i]);
	}
	cout << "init phiAB KinABC success ..." << flush;
	// interior nodes: those need to be solved && inside the artificial boundary
	// interface nodes: those on the artificial boundaries && only used for enforcing the RHS
	b_in = SubArray(b,Nodes_Interior);
	K_in = SubMatrix(K,Nodes_Interior,Nodes_Interior);

	// start timing
	MPI_Barrier(MPI_COMM_WORLD);
	if(myrank==0){
		gettimeofday(&start,NULL);
	}
	int_t* Num_Interior_Nodes_Ranks = (int_t*)malloc(Num_Subdomain * sizeof(int_t));;
	int_t Num_Interior_Nodes = Nodes_Interior.size();
	cout << "starting MPI ASM ..." << flush;
	if(myrank==0){
		
		// The number of elements per message received, not the total number of elements to receive from all processes altogether.
		// For non-root processes, the receiving parameters like this one are ignored.
		MPI_Gather(&Num_Interior_Nodes, 1, MPI_INT, Num_Interior_Nodes_Ranks, 1, MPI_INT, 0, MPI_COMM_WORLD);
		//printf("Values collected on process %d: %d, %d, %d, %d.\n", myrank, Num_Interior_Nodes_Ranks[0], Num_Interior_Nodes_Ranks[1], Num_Interior_Nodes_Ranks[2], Num_Interior_Nodes_Ranks[3]);
		cout << flush;
	}
	else{
		MPI_Gather(&Num_Interior_Nodes, 1, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
	}
	// use reduction to prepare for the gatherv
	int_t Total_Num_Interior_Nodes = 0;
	MPI_Reduce(&Num_Interior_Nodes, &Total_Num_Interior_Nodes, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	cout << "Total_Num_Interior_Nodes " << Total_Num_Interior_Nodes << endl << flush;
	vector<float_t> phi_temp;
	vector<vector<float_t>> enforceABC(Num_Subdomain);
	vector<float_t> r;
	float_t rho, error;
	float_t norm_b = sqrt(INNER_PRODUCT(b,b));
	vector<float_t> rhs;

	for(int asm_iter=0; asm_iter<max_asmiter_num; asm_iter++){
		// solve the sub domain linear system using the interior K and enforced RHS
		cout << "asm_iter " << asm_iter  << endl << flush;
		// calculate RHS enforced by artificial boundary condition
		for(int i=0;i<Num_Subdomain;i++){
			enforceABC[i] = MATRIX_VECTOR_PRODUCT(KinABC[i], phiAB[i]);
		}

		rhs = saxpy(-1,enforceABC[0],b_in);
		for(int i=1;i<Num_Subdomain;i++){
			rhs = saxpy(-1,enforceABC[i],rhs);
		}

		// solve the sub domain matrix (only the nodes at the interior of the domain)
		phi_in = Parallel_CG(K_in,rhs);
		AssignSubArray(&phi,phi_in,Nodes_Interior); // assign back

		// parallel: send the inside nodal coefficient to the neighboring ranks to act as artificial boundary.
		//			 recv from the neighboring ranks and use that for enforcing RHS
		for(int i=0;i<Num_Subdomain;i++){
			buf_send[i]=SubArray(phi,Nodes_othersABC[i]);
		}

		for(int i=0;i<Num_Subdomain;i++){
			//cout << "Send from " << myrank << " " << Nodes_othersABC[i].size() << " MPI_FLOAT to " << Nodes_Interface_ToWhichSubDomain[i] << endl << flush;
			MPI_Isend(&buf_send[i][0], Nodes_othersABC[i].size(), MPI_FLOAT, Nodes_Interface_ToWhichSubDomain[i], 0, MPI_COMM_WORLD, &req[i]);
			//displayVec(buf_send[i]);
			//cout << flush;
		}

		for(int i=0;i<Num_Subdomain;i++){
			//cout << "Recv at " << myrank << " " << Nodes_Interface[i].size() << " MPI_FLOAT from " << Nodes_Interface_FromWhichSubDomain[i] << endl << flush;
			MPI_Irecv(&phiAB[i][0], Nodes_Interface[i].size(), MPI_FLOAT, Nodes_Interface_FromWhichSubDomain[i], 0, MPI_COMM_WORLD, &req[i+ Num_Subdomain]);
			
		}

		// after receive, we can calculate the enforced RHS in the next ASM iteration

		// check convergence on the total domain

		//r = Residual(K,phi,b);
		//rho = INNER_PRODUCT(r,r);
		//error = sqrt(rho)/norm_b;
		// cout << error << endl;
		//cout << "iteration " << asm_iter << " error " << error << endl;
		//if(error < TOL){
			//cout << "The ASM converge at an accuracy of " << error << " at " << asm_iter << " iterations." << endl;
			//break;
		//}

		MPI_Waitall(2*Num_Subdomain, &req[0], MPI_STATUSES_IGNORE);
		/*
		for (int i = 0; i < Num_Subdomain; i++) {
			displayVec(phiAB[i]);
			cout << flush;
		}
		*/
	}
	cout << "ASM end ... " << flush;
	vector<float_t> total_phi_interior_buf(Total_Num_Interior_Nodes,0);
	vector<int_t> total_nodes_interior2global_buf(Total_Num_Interior_Nodes,0);
	cout << "start gathering ... " << flush;
	if(myrank==0){
		cout << "Total_Num_Nodes " << Total_Num_Nodes << " ..." << flush;
		int_t* displacements = (int_t*)calloc(Num_Subdomain,sizeof(int_t));

		for(int dm=1; dm<Num_Subdomain; dm++){
			displacements[dm]+= displacements[dm-1]+Num_Interior_Nodes_Ranks[dm-1];
		}
		MPI_Gatherv(&phi_in[0], Num_Interior_Nodes, MPI_FLOAT, &total_phi_interior_buf[0], Num_Interior_Nodes_Ranks, displacements, MPI_FLOAT, 0, MPI_COMM_WORLD);
		MPI_Gatherv(&Nodes_Interior2Global[0], Num_Interior_Nodes, MPI_INT, &total_nodes_interior2global_buf[0], Num_Interior_Nodes_Ranks, displacements, MPI_INT, 0, MPI_COMM_WORLD);
	}
	else{
		MPI_Gatherv(&phi_in[0], Num_Interior_Nodes, MPI_FLOAT, NULL, NULL, NULL, MPI_FLOAT, 0, MPI_COMM_WORLD);
		MPI_Gatherv(&Nodes_Interior2Global[0], Num_Interior_Nodes, MPI_INT, NULL, NULL, NULL, MPI_INT, 0, MPI_COMM_WORLD);
	}
	cout << "end gathering ... " << flush;
	// end timing
	MPI_Barrier(MPI_COMM_WORLD);

	cout << "ASM MPI end ... " << flush;
	if(myrank==0){
		//displayMat(K_in);

		gettimeofday(&end,NULL);
		float_t run_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
		run_time /= 1000000;
		cout<<" ... MPI ASM FEM costs time "<<run_time<<" s ... "<<endl<<flush;
		vector<float_t> phi_asm(Total_Num_Nodes,0);
		for(int_t i=0;i<Total_Num_Interior_Nodes;i++){
			phi_asm[total_nodes_interior2global_buf[i]]=total_phi_interior_buf[i];
		}
		WriteOutput(phi_asm);
	}


	// the linear system has a length of the interior node numbers, do not include the artificial boundary
	// there will be a separate array for artificial boundary nodes of each sub domain. They will be used as the MPI receiving buffer as well as the array to enforce artificial boundary.
	// after receive the other ranks, the
	// each rank also acts as the sender, so the rank need to know which nodal coefficients to send

	// although the linear system has a length of interior node numbers, the assembly procedure should be done for the discretized region including the interface boundary.


	// MPI Gather?

	MPI_Finalize();
	//WriteOutput(phi_total)
	return 0;
}


