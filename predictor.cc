

#include "predictor.h"

#define HIST_LEN_1 130
#define HIST_LEN_2  44
#define HIST_LEN_3  15
#define HIST_LEN_4  5

#define TAG_LEN_1 12
#define TAG_LEN_2 12
#define TAG_LEN_3 12
#define TAG_LEN_4 11

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

PREDICTOR::PREDICTOR(void){
   
	phist = 0;

	bimodal_len = (1 << BIMODAL_BITS);
	bimodal = new bimodal_t[bimodal_len];

	

  bimodal[0].ctr = BIMODAL_PRED_CTR_INIT;
  bimodal[1].ctr = BIMODAL_PRED_CTR_INIT;
  bimodal[2].ctr = BIMODAL_PRED_CTR_INIT;
  bimodal[3].ctr = BIMODAL_PRED_CTR_INIT;



	// Init the tag tables and corresponding helper tables.
	tage_len = (1 << TAGE_BITS);

	for (int i = 0; i < NUMBER_OF_TAGE ; i++) 
  {
		tageOfTables[i] = new global_t[tage_len];

		for (UINT32 j = 0; j < tage_len; j++)
    {
			tageOfTables[i][j].ctr = TAGE_PRED_CTR_INIT;
			tageOfTables[i][j].tag = 0;
			tageOfTables[i][j].u = 0;
		}

		tage_index[i] = 0 ;
		tage_tag[i] = 0 ;
	}


  tag_width[0] = TAG_LEN_1 ;
  tag_width[1] = TAG_LEN_2 ;
  tag_width[2] = TAG_LEN_3 ;
  tag_width[3] = TAG_LEN_4 ;

  history_length[0] =  HIST_LEN_1 ;
  history_length[1] =  HIST_LEN_2 ;
  history_length[2] =  HIST_LEN_3 ;
  history_length[3] =  HIST_LEN_4 ;


  for (int i = 0; i < NUMBER_OF_TAGE ; ++i)
  {
    /* code */

    //FoldingInit(&folded_index[i], history_length[i], TAGE_BITS);
    //FoldingInit(&folded_tag[0][i], history_length[i], tag_width[i]);
    //FoldingInit(&folded_tag[1][i], history_length[i], tag_width[i] - 1);

    //f->comp = 0;
    //f->orig_length = original_length;
    //f->comp_length = compressed_length;

    folded_index[i].comp = 0 ;
    folded_index[i].orig_length = history_length[i] ;
    folded_index[i].comp_length = TAGE_BITS ;

    folded_tag[0][i].comp = 0 ;
    folded_tag[0][i].orig_length = history_length[i] ;
    folded_tag[0][i].comp_length = tag_width[i] ;

    folded_tag[1][i].comp = 0 ;
    folded_tag[1][i].orig_length = history_length[i] ;
    folded_tag[1][i].comp_length = tag_width[i] - 1 ;


  }

	use_alt = USE_ALT_CTR_INIT;
	tick = 0;
	toggle = 1;

  pred.primary = -1;
  pred.alt = -1;
  pred.primary_table = NUMBER_OF_TAGE;
  pred.alt_table = NUMBER_OF_TAGE;

  globalHistory.reset();

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool   PREDICTOR::GetPrediction(UINT32 PC) {

    
    pred.primary = -1;
    pred.alt = -1;
    pred.primary_table = NUMBER_OF_TAGE;
    pred.alt_table = NUMBER_OF_TAGE;

    // Get the Index and tags for the current branch.
    for (int i = 0 ; i < NUMBER_OF_TAGE; i++) 
    {
        
        int tag = (PC ^ folded_tag[0][i].comp ^ (folded_tag[1][i].comp << 1));
        tage_tag[i] = (tag & ((1 << tag_width[i]) - 1));

        //tage_tag[i] = GetTag(PC, i, tag_width[i]);

        int index = PC ^ (PC >> (TAGE_BITS - i )) ^ folded_index[i].comp ^ (phist>> (TAGE_BITS - i));
        tage_index[i] =   (index & ((1 << TAGE_BITS) - 1));

        //tage_index[i] = GetIndex(PC, i);
    }

    for (int i = 0; i < NUMBER_OF_TAGE; i++)
     {
        if (tageOfTables[i][tage_index[i]].tag == tage_tag[i]) {
            // Found primary prediction.
            pred.primary_table = i;
            break;
        }
    }
    
    for (int i = pred.primary_table + 1; i < NUMBER_OF_TAGE; i++) {
        if (tageOfTables[i][tage_index[i]].tag == tage_tag[i]) {
            // Found an alternate prediction.
            pred.alt_table = i;
            break;
        }
    }

    // at the end get the bimodal prediction.
    bimodalIndex = (PC & ((1 << BIMODAL_BITS ) - 1));

    if (bimodal[bimodalIndex].ctr > BIMODAL_PRED_CTR_MAX/2) {
        pred.bimodal =  TAKEN;
    } else {
        pred.bimodal =  NOT_TAKEN;
    }

    if (pred.primary_table == NUMBER_OF_TAGE) {
        
        return pred.bimodal;
    } else 
    {
        if (pred.alt_table == NUMBER_OF_TAGE) 
        {
            pred.alt = pred.bimodal;
        } 
        else 
        {
            if (tageOfTables[pred.alt_table][tage_index[pred.alt_table]].ctr
                >= TAGE_PRED_CTR_MAX/2) 
            {
                pred.alt = TAKEN;
            } else {
                pred.alt = NOT_TAKEN;
            }
        }

        if ((tageOfTables[pred.primary_table][tage_index[pred.primary_table]].ctr  != WEAK_NOT_TAKEN) ||
            (tageOfTables[pred.primary_table][tage_index[pred.primary_table]].ctr != WEAK_TAKEN) ||
            (tageOfTables[pred.primary_table][tage_index[pred.primary_table]].u != 0) ||
            (use_alt < 8)) 
        {
            if (tageOfTables[pred.primary_table][tage_index[pred.primary_table]].ctr
                > TAGE_PRED_CTR_MAX/2) 
            {
                pred.primary = TAKEN;
            } else  {
                pred.primary = NOT_TAKEN;
            }
            return pred.primary;
        } else {
            // In case of a weak entry take the alternate
            // prediction.
            return pred.alt;
        }
    }
}


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void  PREDICTOR::UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

    
 	

 	if (pred.primary_table == NUMBER_OF_TAGE) {
 		// We have only bimodal prediction.
 		if(resolveDir == TAKEN) 
    {
 			
      if (bimodal[bimodalIndex].ctr < BIMODAL_PRED_CTR_MAX )
      {
        /* code */
        bimodal[bimodalIndex].ctr = bimodal[bimodalIndex].ctr + 1 ;
      }

 		} 
    else 
    {
 			
      if (bimodal[bimodalIndex].ctr  > 0)
      {
        /* code */
        bimodal[bimodalIndex].ctr = bimodal[bimodalIndex].ctr - 1;
      }
 		}

 	} else 
  {
 		// Update the counters in the primary table.
 		if (resolveDir == TAKEN) 
    {
 			tageOfTables[pred.primary_table][tage_index[pred.primary_table]].ctr =
 			    SatIncrement(tageOfTables[pred.primary_table][tage_index[pred.primary_table]].ctr,
 			    TAGE_PRED_CTR_MAX);
 		} else 
    {
 			tageOfTables[pred.primary_table][tage_index[pred.primary_table]].ctr =
 			SatDecrement(tageOfTables[pred.primary_table][tage_index[pred.primary_table]].ctr);
 		}

 		// Update the useful counters of the primary table
 		if (pred.primary == resolveDir) 
    {
 			
        if ( tageOfTables[pred.primary_table][tage_index[pred.primary_table]].u < TAGE_USEFUL_CTR_MAX )
        {
          /* code */

          tageOfTables[pred.primary_table][tage_index[pred.primary_table]].u = tageOfTables[pred.primary_table][tage_index[pred.primary_table]].u + 1;
        }

 		} else {
 		
          if ( tageOfTables[pred.primary_table][tage_index[pred.primary_table]].u > 0)
          {
            /* code */
            tageOfTables[pred.primary_table][tage_index[pred.primary_table]].u = tageOfTables[pred.primary_table][tage_index[pred.primary_table]].u - 1;
          }


 		}

 	
 		if (((tageOfTables[pred.primary_table][tage_index[pred.primary_table]].ctr == WEAK_NOT_TAKEN) ||
 		    (tageOfTables[pred.primary_table][tage_index[pred.primary_table]].ctr == WEAK_TAKEN)) &&
 		    (tageOfTables[pred.primary_table][tage_index[pred.primary_table]].u == 0)) 
    {
 			if ((pred.primary != pred.alt) && (pred.alt == resolveDir)) 
      {
 				SatIncrement(use_alt, USE_ALT_CTR_MAX);

 				if (pred.alt_table != NUMBER_OF_TAGE) {
 					if (resolveDir == TAKEN) 
          {

            if ( tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u < TAGE_USEFUL_CTR_MAX )
            {
              /* code */
              tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u = tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u + 1;
            }

 					} else {
 						

            if ( tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u > 0)
            {
              /* code */
              tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u = tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u - 1;
            }

 					}

          if ( tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u < TAGE_USEFUL_CTR_MAX )
          {
            /* code */
            tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u = tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u + 1 ;

          }

 				}
 			} else 
      {
 				SatDecrement(use_alt);
 				if (pred.alt_table != NUMBER_OF_TAGE) {
 				

          if (tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u > 0)
          {
            /* code */
            tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u = tageOfTables[pred.alt_table][tage_index[pred.alt_table]].u - 1 ;
          }

 				}
 			}
 		}
 	}

  if ((pred.primary_table > 0) && (predDir != resolveDir)) 
  {
    bool found_empty = false;
    for (int i = 0; i < pred.primary_table; i++) 
    {
      if (tageOfTables[i][tage_index[i]].u == 0)
       {
        found_empty = true;
        break;
      }
    }

    if (found_empty == true) {
      int random_num = rand() % 100;
      for (int i = pred.primary_table - 1; i >= 0; i--)
       {
        if (tageOfTables[i][tage_index[i]].u == 0) 
        {
                 
          if (random_num <= 99/ ( pred.primary_table - i+1)) 
          {
            if (resolveDir == TAKEN) {
              tageOfTables[i][tage_index[i]].ctr = 4;
            } else {
              tageOfTables[i][tage_index[i]].ctr = 3;
            }
            tageOfTables[i][tage_index[i]].tag = tage_tag[i];
            tageOfTables[i][tage_index[i]].u = 0;
          }
        }
      }
     } else 
     {
             // Choose a random table and steal it.
       int i = rand() % NUMBER_OF_TAGE;

       if(resolveDir == TAKEN) 
       {
         tageOfTables[i][tage_index[i]].ctr = WEAK_TAKEN;
       } else {
         tageOfTables[i][tage_index[i]].ctr = WEAK_NOT_TAKEN;
       }
       tageOfTables[i][tage_index[i]].tag = tage_tag[i];
       tageOfTables[i][tage_index[i]].u = 0;
     }
  }

 	// update the Global history and path history.
 	globalHistory = (globalHistory << 1);
 	phist = (phist << 1);
 	if (resolveDir == TAKEN) {
 		globalHistory.set(0,1);
 		phist = phist + 1;
 	}

  Folding(&folded_index[0], globalHistory);
  Folding(&folded_index[1], globalHistory);
  Folding(&folded_index[2], globalHistory);
  Folding(&folded_index[3], globalHistory);

 	phist = phist % PATH_HISTORY_LEN;

 	for (int i = 0; i < NUMBER_OF_TAGE; ++i) {
 		
 		Folding(&folded_tag[0][i], globalHistory);
 		Folding(&folded_tag[1][i], globalHistory);
 	}

 	tick++;

  UINT32 maxclock ;
  maxclock = ( 1 << MAX_CLOCK) ;

 	if (tick == maxclock) 
  { 
 		if (toggle == 1) {
 			for (int i = 0; i < NUMBER_OF_TAGE; ++i) {
 				for (UINT32 j = 0; j < tage_len; ++j) {
 					tageOfTables[i][j].u = tageOfTables[i][j].u & 1;
 				}
 			}
 			toggle = 0 ;
 		} else {
 			for (int i = 0; i < NUMBER_OF_TAGE; i++) {
 				for (UINT32 j = 0; j < tage_len; j++) {
 					tageOfTables[i][j].u = tageOfTables[i][j].u & 2;
 				}
 			}
 			toggle = 1;
 		}
 		tick = 0;
 	}
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void    PREDICTOR::TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget) {

  // This function is called for instructions which are not
  // conditional branches, just in case someone decides to design
  // a predictor that uses information from such instructions.
  // We expect most contestants to leave this function untouched.

  return;
}


void PREDICTOR::FoldingInit(fold_history_t *f,  UINT32 original_length,
    UINT32 compressed_length) 
{
	f->comp = 0;
	f->orig_length = original_length;
	f->comp_length = compressed_length;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////


void PREDICTOR::Folding(fold_history_t *fold, GHIST globalHistory) 
{
	fold->comp = (fold->comp << 1) | globalHistory[0];
	fold->comp ^= globalHistory[fold->orig_length] << (fold->orig_length % fold->comp_length);
	fold->comp ^= (fold->comp >> fold->comp_length);
	fold->comp &= (1 << fold->comp_length) - 1;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

