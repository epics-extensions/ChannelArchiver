/*
 *	This file contains all the parameters that are used by individual index classes
*/

#ifndef _R_TREE_CONSTANTS_H_
#define _R_TREE_CONSTANTS_H_

//general
#define MAX_RAM					100000	//RAM memory in bytes that can be allocated	
#define COMPLETE_TIME_RANGE     0,1,0,0	//the biggest possible interval
#define NULL_INTERVAL			0,0,0,0

//key_object
#define KO_OFFSET_OFFSET		0
#define KO_PATH_LENGTH_OFFSET	4
#define KO_PATH_OFFSET			6

#define KO_SIZE_WITHOUT_PATH	6


//interval
#define IV_START_SEC_OFFSET		0
#define IV_START_NSEC_OFFSET	4
#define IV_END_SEC_OFFSET		8
#define	IV_END_NSEC_OFFSET		12

//for printing
//CAUTION! If changed, must be changed in interval::print()
#define TIME_STRING_LENGTH		2
#define IV_STRING_LENGTH		(TIME_STRING_LENGTH * 4 + 5)

//r_tree_root
#define ROOT_IV_OFFSET			0
#define	ROOT_CHILD_OFFSET		16
#define	ROOT_LATEST_OFFSET		20
#define ROOT_I_OFFSET			24	//short

#define ROOT_SIZE				(ROOT_I_OFFSET + 2)

//rtfsm
#define RTFSM_HEADER_OFFSET		0
#define RTFSM_REG_OFFSET		RTFSM_HEADER_OFFSET	
#define RTFSM_BYTE_ARRAY_OFFSET	4
#define RTFSM_HEADER_SIZE		RTFSM_BYTE_ARRAY_OFFSET

//AU
#define AU_IV_OFFSET			0
#define AU_PRIORITY_OFFSET		16
#define AU_NEXT_OFFSET			18
#define AU_PREVIOUS_OFFSET		22
#define AU_KEY_OFFSET			26
//size of the au
#define AU_SIZE_WITHOUT_KEY		26		

//r_entry
#define ENTRY_IV_OFFSET			0
#define ENTRY_CHILD_OFFSET		16
#define ENTRY_PARENT_OFFSET		20
#define	ENTRY_NEXT_OFFSET		24
#define ENTRY_PREVIOUS_OFFSET	28
#define ENTRY_KEY_OFFSET		32

#define	ENTRY_SIZE				36

//cntu
#define CNTU_NAME_OFFSET		0
#define CHANNEL_NAME_LENGTH		200	//including 0 termination!!!
#define CNTU_ROOT_OFFSET		200
#define CNTU_AU_LIST_OFFSET		204
#define CNTU_NEXT_OFFSET		208
#define CNTU_PREVIOUS_OFFSET	212

#define CNTU_SIZE				216

//index 
#define MAGIC_ID					0
#define MAGIC_ID_SIZE				4
#define CNTU_TABLE_POINTER			MAGIC_ID_SIZE
#define HASH_TABLE_SIZE				(MAGIC_ID_SIZE + 4)
#define R_TREE_M					(MAGIC_ID_SIZE + 6)

#define HEADER_SIZE					(MAGIC_ID + MAGIC_ID_SIZE + 8)


inline long twoToThePowerOf(int i)	{return (1 << i);	}

#endif //rtree_constants.h

