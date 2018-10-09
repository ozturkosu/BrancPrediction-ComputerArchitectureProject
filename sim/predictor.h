#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

// Modified TAGE predictor

#include "utils.h"
#include "tracer.h"
#include <bitset>

#define UINT16 unsigned short int

// Global History //
#define GHIST_MAX_LEN 131 
#define GHIST bitset<GHIST_MAX_LEN>

#define PATH_HISTORY_LEN 16 
#define USE_ALT_CTR_INIT 8
#define USE_ALT_CTR_MAX 15


#define BIMODAL_BITS   13
#define BIMODAL_PRED_CTR_INIT 2
#define BIMODAL_PRED_CTR_MAX  3 


#define WEAK_NOT_TAKEN	3
#define WEAK_TAKEN	4

#define NUMBER_OF_TAGE 4	
#define TAGE_BITS 11	
#define TAGE_PRED_CTR_INIT 0
#define TAGE_PRED_CTR_MAX  7 
#define TAGE_USEFUL_CTR_MAX 3

#define MAX_CLOCK 18



typedef struct global {
    UINT32 ctr; // Prediction counter 3 bits.
    UINT32 tag; // Tag for each entry
    UINT32 u; // Useful bits. L-TAGE paper says use 2 bits.
} global_t;

typedef struct fold_history {

	UINT32 comp;
	UINT32 comp_length;
	UINT32 orig_length;
} fold_history_t;

typedef struct prediction {
	
	bool bimodal;
	
	bool primary;
	int primary_table;
	
	bool alt;
	int alt_table;

  UINT32 index ;
  UINT32 alt_table_index ;

} prediction_t;

typedef struct bimodal {
  UINT32 ctr; 

} bimodal_t;





/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

class PREDICTOR {

  // The state is defined for Gshare, change for your design

 private:
     

     GHIST globalHistory ;
     UINT16 phist; // 

     prediction_t pred;
     UINT32 use_alt; // Global 4 bit counter to check whether to use alternate prediction or not.
     
     bimodal_t *bimodal;
     UINT32 bimodalIndex;
     UINT32 bimodal_len;

    
     global_t *tageOfTables[NUMBER_OF_TAGE];
     UINT32 tage_len;
     UINT32 history_length[NUMBER_OF_TAGE];
     UINT32 tag_width[NUMBER_OF_TAGE];

     
     fold_history_t folded_index[NUMBER_OF_TAGE];
     fold_history_t folded_tag[2][NUMBER_OF_TAGE];

     // Store the index and tags for each table.

     UINT32 tage_index[NUMBER_OF_TAGE];
     UINT32 tage_tag[NUMBER_OF_TAGE];

     bool toggle;
     UINT32 tick;

 public:
     // The interface to the four functions below CAN NOT be changed
     PREDICTOR(void);
     bool    GetPrediction(UINT32 PC);
     void    UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget);
     void    TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget) ;

     //void Folding(fold_history *f, GHIST globalHistory);
     //void FoldingInit(fold_history_t *f,  UINT32 original_length,
     //void PREDICTOR::Folding(fold_history_t *fold, GHIST globalHistory) 
     
};

/***********************************************************/
#endif