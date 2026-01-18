#define L1  0.088
#define L2  0.15
//#define L3  0.11
#define L3  0.06

//#define L1  0.14
//#define L2  0.20
//#define L3  0.20

float *q = (float*)malloc(3*sizeof(float));
float *p = (float*)malloc(3*sizeof(float));

float *MCD(float *q){
  p[0] = L2*cos(q[0])*cos(q[1])+L3*cos(q[0])*cos(q[1]+q[2]);
  p[1] = L2*cos(q[1])*sin(q[0])+L3*sin(q[0])*cos(q[1]+q[2]);
  p[2] = L1+L2*sin(q[1])+L3*sin(q[1]+q[2]);

   
  return p;  
}


float *MCI(float *p){
  float D = (p[0]*p[0] + p[1]*p[1] + pow((p[2] - L1), 2) - pow(L2,2) - pow(L3,2)) / (2 * L2 * L3);
  q[0] = atan2(p[1],p[0]);
  q[2] = atan2(sqrt(1-pow(D,2)),D);
  q[1] = atan2(p[2]-L1,sqrt(pow(p[0],2)+pow(p[1],2))) - atan2(L3*sin(q[2]), L2+L3*cos(q[2]));
  return q;  
}
