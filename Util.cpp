#include "pch.h"

#include "Util.h"


void quickSortSwap(SorterEntry *a, SorterEntry *b ){
  SorterEntry temp;
  temp = *a;
  *a=*b;
  *b=temp;
}

void quickSortF(SorterEntry array[],  int left ,int right){
    float center;
    int l,r;
    if(left <= right){
        center = array[ ( left + right ) / 2 ].val;
        l = left;
        r = right;
        while(l <= r){
            while(array[l].val < center)l++;
            while(array[r].val > center)r--;
            if(l <= r){
                quickSortSwap(&array[l],&array[r]);
                l++;
                r--;
            }
        }
        quickSortF(array, left, r);
        quickSortF(array, l, right);
    }
}

