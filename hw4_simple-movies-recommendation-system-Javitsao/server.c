#include "header.h"

movie_profile* movies[MAX_MOVIES];
unsigned int num_of_movies = 0;
unsigned int num_of_reqs = 0;

// Global request queue and pointer to front of queue
// TODO: critical section to protect the global resources
request* reqs[MAX_REQ];
int front = -1;
int t_i[MAX_THREAD];
/* Note that the maximum number of processes per workstation user is 512. * 
 * We recommend that using about <256 threads is enough in this homework. */ 
pthread_t tid[MAX_THREAD]; //tids for multithread

#ifdef PROCESS
pid_t pid[MAX_CPU][MAX_THREAD]; //pids for multiprocess
#endif
//mutex
pthread_mutex_t lock; 
void initialize(FILE* fp);
request* read_request();
int pop();
int pop(){
	front+=1;
	return front;
}
typedef struct Var{
	int idx;
	int l;
	int r;
	int depth;
}Var;
#ifdef THREAD
	char *filt_mov[MAX_REQ][MAX_MOVIES];
	double filt_pts[MAX_REQ][MAX_MOVIES];
#endif 
#ifdef PROCESS	
	char **filt_mov[MAX_REQ];
	double *filt_pts[MAX_REQ];
	char **tmp;
#endif 

void merge(int l, int mid, int r, int idx);
void* mergeSort(void *arg);
void* t_request(void *arg);

int main(int argc, char *argv[]){
	if(argc != 1){
#ifdef PROCESS
		fprintf(stderr,"usage: ./pserver\n");
#elif defined THREAD
		fprintf(stderr,"usage: ./tserver\n");
#endif
		exit(-1);
	}
	FILE *fp;
	if ((fp = fopen("./data/movies.txt","r")) == NULL){
		ERR_EXIT("fopen");
	}
	initialize(fp);
	//printf("nor = %d\n", num_of_reqs);
	assert(fp != NULL);
#ifdef PROCESS	
	for(int i = 0; i < num_of_reqs; i++){
		filt_mov[i] = mmap(NULL, sizeof(char *) * MAX_MOVIES, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		filt_pts[i] = mmap(NULL, sizeof(double) * MAX_MOVIES, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	}
	//filt_mov[MAX_REQ] = mmap(NULL, sizeof(char *) * MAX_MOVIES, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	//filt_pts[MAX_REQ] = mmap(NULL, sizeof(double) * MAX_MOVIES, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	t_request((void*)0);
#endif

#ifdef THREAD	
	for(int i = 0; i < num_of_reqs; i++){
		//t_i[i] = i;
		pthread_create(&tid[i], NULL, t_request, (void*)i);
	}
	for(int i = 0; i < num_of_reqs; i++){
		pthread_join(tid[i], NULL);
	}
#endif
	fclose(fp);	
	return 0;
}

void merge(int l, int mid, int r, int idx)
{
    int len_1 = mid - l + 1, len_2 = r - mid;
    double left[len_1], right[len_2];
	char *left_mov[len_1], *right_mov[len_2];
    for(int p = 0; p < len_1; p++){
		left[p] = filt_pts[idx][l + p];
		left_mov[p] = filt_mov[idx][l + p];
	}
    for(int p = 0; p < len_2; p++){
		right[p] = filt_pts[idx][mid + 1 + p];
		right_mov[p] = filt_mov[idx][mid + 1 + p];
	}
    int l_ptr = 0, r_ptr = 0, cnt = l;
    while(l_ptr < len_1 && r_ptr < len_2){
        if (left[l_ptr] > right[r_ptr]){
			filt_pts[idx][cnt] = left[l_ptr];
			filt_mov[idx][cnt] = left_mov[l_ptr];
            l_ptr++;
        }
		else if(left[l_ptr] == right[r_ptr]){
			if(strcmp(left_mov[l_ptr], right_mov[r_ptr]) > 0){
				filt_pts[idx][cnt] = right[r_ptr];
				filt_mov[idx][cnt] = right_mov[r_ptr];
				r_ptr++;
			}
			else{
				filt_pts[idx][cnt] = left[l_ptr];
				filt_mov[idx][cnt] = left_mov[l_ptr];
				l_ptr++;
			}
		}
        else{
			filt_pts[idx][cnt] = right[r_ptr];
			filt_mov[idx][cnt] = right_mov[r_ptr];
            r_ptr++;
        }
        cnt++;
    }
    while(l_ptr < len_1){
		filt_pts[idx][cnt] = left[l_ptr];
		filt_mov[idx][cnt] = left_mov[l_ptr];
        l_ptr++; cnt++;
    }
    while(r_ptr < len_2){
		filt_pts[idx][cnt] = right[r_ptr];
		filt_mov[idx][cnt] = right_mov[r_ptr];
        r_ptr++; cnt++;
    }

}
void* mergeSort(void* arg)
{
    Var var = *((Var*)arg);
    int l = var.l;
    int r = var.r;
	//printf("l = %d r = %d depth = %d\n", l, r, var.depth);
    if(var.depth < 4 && r - l > 600){
		if (l < r)
		{
			int mid = l + (r - l) / 2;
			Var var_left = {var.idx, l, mid, var.depth + 1};
			Var var_right = {var.idx, mid + 1, r, var.depth + 1};
		#ifdef THREAD
			pthread_t thread1;
			pthread_t thread2;
			pthread_create(&thread1, NULL, mergeSort, &var_left);
			pthread_create(&thread2, NULL, mergeSort, &var_right);
		#endif
		#ifdef PROCESS
			int pid_l, pid_r;
			if((pid_l = fork()) <= 0){
				mergeSort(&var_left);
				_exit(0);
			}
			waitpid(pid_l, NULL, 0);
			if((pid_r = fork()) <= 0){
				mergeSort(&var_right);
				_exit(0);
			}
			waitpid(pid_r, NULL, 0);
		#endif
		#ifdef THREAD
			pthread_join(thread1, NULL);
			pthread_join(thread2, NULL);
		#endif
			merge(l, mid, r, var.idx);
		}
		if(l == r){
			sort(filt_mov[var.idx] + l, filt_pts[var.idx] + l, r - l + 1);
		#ifdef PROCESS
			for(int i = 0; i < r - l + 1; i++){
				strcpy(tmp[l + i], filt_mov[var.idx][i + l]);
				filt_mov[var.idx][i + l] = tmp[l + i];
			}
		#endif
		}
	}
	else{
		sort(filt_mov[var.idx] + l, filt_pts[var.idx] + l, r - l + 1);
	#ifdef PROCESS
		for(int i = 0; i < r - l + 1; i++){
			strcpy(tmp[l + i], filt_mov[var.idx][i + l]);
			filt_mov[var.idx][i + l] = tmp[l + i];
		}
	#endif
	}
}
/**=======================================
 * You don't need to modify following code *
 * But feel free if needed.                *
 =========================================**/

request* read_request(){
	int id;
	char buf1[MAX_LEN],buf2[MAX_LEN];
	char delim[2] = ",";

	char *keywords;
	char *token, *ref_pts;
	char *ptr;
	double ret,sum;

	scanf("%u %254s %254s",&id,buf1,buf2);
	keywords = malloc(sizeof(char)*strlen(buf1)+1);
	if(keywords == NULL){
		ERR_EXIT("malloc");
	}

	memcpy(keywords, buf1, strlen(buf1));
	keywords[strlen(buf1)] = '\0';

	double* profile = malloc(sizeof(double)*NUM_OF_GENRE);
	if(profile == NULL){
		ERR_EXIT("malloc");
	}
	sum = 0;
	ref_pts = strtok(buf2,delim);
	for(int i = 0;i < NUM_OF_GENRE;i++){
		ret = strtod(ref_pts, &ptr);
		profile[i] = ret;
		sum += ret*ret;
		ref_pts = strtok(NULL,delim);
	}

	// normalize
	sum = sqrt(sum);
	for(int i = 0;i < NUM_OF_GENRE; i++){
		if(sum == 0)
				profile[i] = 0;
		else
				profile[i] /= sum;
	}

	request* r = malloc(sizeof(request));
	if(r == NULL){
		ERR_EXIT("malloc");
	}

	r->id = id;
	r->keywords = keywords;
	r->profile = profile;

	return r;
}

/*=================initialize the dataset=================*/
void initialize(FILE* fp){

	char chunk[MAX_LEN] = {0};
	char *token,*ptr;
	double ret,sum;
	int cnt = 0;

	assert(fp != NULL);

	// first row
	if(fgets(chunk,sizeof(chunk),fp) == NULL){
		ERR_EXIT("fgets");
	}

	memset(movies,0,sizeof(movie_profile*)*MAX_MOVIES);	

	while(fgets(chunk,sizeof(chunk),fp) != NULL){
		
		assert(cnt < MAX_MOVIES);
		chunk[MAX_LEN-1] = '\0';

		const char delim1[2] = " "; 
		const char delim2[2] = "{";
		const char delim3[2] = ",";
		unsigned int movieId;
		movieId = atoi(strtok(chunk,delim1));

		// title
		token = strtok(NULL,delim2);
		char* title = malloc(sizeof(char)*strlen(token)+1);
		if(title == NULL){
			ERR_EXIT("malloc");
		}
		
		// title.strip()
		memcpy(title, token, strlen(token)-1);
	 	title[strlen(token)-1] = '\0';

		// genres
		double* profile = malloc(sizeof(double)*NUM_OF_GENRE);
		if(profile == NULL){
			ERR_EXIT("malloc");
		}

		sum = 0;
		token = strtok(NULL,delim3);
		for(int i = 0; i < NUM_OF_GENRE; i++){
			ret = strtod(token, &ptr);
			profile[i] = ret;
			sum += ret*ret;
			token = strtok(NULL,delim3);
		}

		// normalize
		sum = sqrt(sum);
		for(int i = 0; i < NUM_OF_GENRE; i++){
			if(sum == 0)
				profile[i] = 0;
			else
				profile[i] /= sum;
		}

		movie_profile* m = malloc(sizeof(movie_profile));
		if(m == NULL){
			ERR_EXIT("malloc");
		}

		m->movieId = movieId;
		m->title = title;
		m->profile = profile;

		movies[cnt++] = m;
	}
	num_of_movies = cnt;

	// request
	scanf("%d",&num_of_reqs);
	assert(num_of_reqs <= MAX_REQ);
	for(int i = 0; i < num_of_reqs; i++){
		reqs[i] = read_request();
	}
}
/*========================================================*/
