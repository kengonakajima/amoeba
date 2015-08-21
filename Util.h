// copied from cumino.h

inline float maxf( float a, float b ){
    return (a>b) ? a:b;
}
inline float maxf( float a, float b, float c ) {
    return maxf( maxf(a,b), c );
}
inline float maxf( float a, float b, float c, float d ) {
    return maxf( maxf(a,b), maxf(c,d) );
}
inline double maxd( double a, double b ){
    return  (a>b) ? a:b;
}
inline double mind( double a, double b ){
    return (a<b) ? a:b;
}
inline int maxi( int a, int b ){
    return  (a>b) ? a:b;
}
inline int mini( int a, int b ){
    return (a<b) ? a:b;
}
inline long random() { 
	int top = rand();
	int bottom = rand();
	long out = ( top << 16 ) | bottom;
	return out;
}

inline double range( double a, double b ) {
    long r = random();
    double rate = (double)r / (double)(0x7fffffff);
    double _a = mind(a,b);
    double _b = maxd(a,b);
    return _a + (_b-_a)*rate;
}
inline int irange( int a, int b ) {
    double r = range(a,b);
    return (int)r;
}


void print( const char *fmt, ... );


inline float len(float x0, float y0, float x1, float y1 ){
    return sqrt( (x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));
}
inline float len(float x, float y ) {
    return sqrt( x*x + y*y);
}

class SorterEntry {
public:
    float val;
    void *ptr;
    SorterEntry(float v, void *p) : val(v), ptr(p){}
    SorterEntry() : val(0), ptr(0) {}
};
void quickSortF(SorterEntry array[], int left ,int right);
