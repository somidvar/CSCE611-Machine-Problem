/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"


/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/
int ContFramePool::poolCounter=0;
ContFramePool* ContFramePool::pools[] ={};

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    baseFrameNum = _base_frame_no;//512 for kernel which is located at 2 MB
    numofFrame = _n_frames;//512 which translates into 2 MB located between 2MB and 4 MB
    infoFrameNum = _info_frame_no;//Location of the manager
    numofInfoFrames = _n_info_frames;//Number of frames required for the frame manager

    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(infoFrameNum == 0) {
        bitmap = (unsigned char *) (baseFrameNum * FRAME_SIZE); // this is the frame manager for only the kernel
    } else {
        bitmap = (unsigned char *) (infoFrameNum * FRAME_SIZE); //this is the frame manager for only the process
    }
    
    // To avoid internal fragmentation, the pool number of frames should be dividable by 8 to fill one byte
    assert ((numofFrame % 8 ) == 0);
    
    //Initiliziation which makes all the bitmap 1 and all the head 0
    for(int i=0; i< numofFrame/4; i++) {
        freeFrameNum+=4;
        bitmap[i] = 85; //where the even bits are availibility and odd ones are the head
    }
    Console::puts("Number of free frames: ");
    Console::putui(freeFrameNum);
    Console::puts("\n");
    // Mark the first frame as being used if it is being used and as a head
    if(infoFrameNum == 0) {
        bitmap[0] = 86;
        freeFrameNum--;
    }
    Console::puts("Frame Pool is successfully initialized\n");

    int dataBits[8];
    for (int bitCounter = 0 ; bitCounter != 8 ; bitCounter++) {
        dataBits[bitCounter] = (bitmap[0] & (1 << bitCounter)) != 0;
        Console::puti(dataBits[bitCounter]);
    }
    Console::puts("\n");
    
    pools[poolCounter]= this;
    poolCounter++;
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames){
    unsigned long requestedFrameNum = _n_frames;//number of frames needed for the process or kernel
    assert(requestedFrameNum<freeFrameNum);

    bool foundFlag=false;
    bool searchContinue=true;

    unsigned long moreFrame;
    unsigned long searchCounter=0;
    unsigned long baseAdd=0;
    while(true){
        int charPage=int(searchCounter/8);
        int charRemainder=(searchCounter%8);

        int dataBits[8];
        for (int bitCounter = 0 ; bitCounter != 8 ; bitCounter++) {
            dataBits[bitCounter] = (bitmap[charPage] & (1 << bitCounter)) != 0;
        }
        for(int j=charRemainder;j<8;j+=2){
            if(dataBits[j]==0){
                moreFrame=requestedFrameNum;
                baseAdd=searchCounter+2;
                break;
            }
            else{
                moreFrame--;
                if(moreFrame==0){
                    foundFlag=true;
                    break;
                }
            }
            // if (j==6)
            //     searchCounter=(charPage+1)*8;
        }
        if(foundFlag)
            break;
        if(searchCounter==numofFrame)
            break;
        searchCounter+=2;
    }

    if(foundFlag){//updating the head
        int charPage=int((baseAdd+1)/8);
        int charRemainder=(baseAdd+1)%8;
        int dataBits[8];
        for (int bitCounter = 0 ; bitCounter != 8 ; bitCounter++) {
            dataBits[bitCounter] = (bitmap[charPage] & (1 << bitCounter)) != 0;
        }
        dataBits[charRemainder]=1;

        int tempVal=0;
        int pow=1;
        int counter=0;
        for(int i=0;i<8;i++){
            tempVal+=pow*dataBits[counter];
            pow*=2;
            counter++;
        }
        bitmap[charPage]=tempVal;
    }

    if(foundFlag){//updating availibility
        unsigned long tempBegin=baseAdd;
        unsigned long tempEnd=-1;
        moreFrame=requestedFrameNum; 
        while(true){
            int charPage=int(tempBegin/8);
            int charRemainder=tempBegin%8;
            int dataBits[8];
            for (int bitCounter = 0 ; bitCounter != 8 ; bitCounter++) {
                dataBits[bitCounter] = (bitmap[charPage] & (1 << bitCounter)) != 0;
            }
            if(charRemainder+2*moreFrame>=8)
                tempEnd=8;
            else
                tempEnd=charRemainder+2*moreFrame;
            for(int i=charRemainder;i<tempEnd;i+=2){
                dataBits[i]=0;
                moreFrame--;
                tempBegin+=2;
            }
            int tempVal=0;
            int pow=1;
            int counter=0;
            for(int i=0;i<8;i++){
                tempVal+=pow*dataBits[counter];
                pow*=2;
                counter++;
            }
            bitmap[charPage]=tempVal;
            if(moreFrame==0)
                break;
        }
    }
    if(foundFlag){
        baseAdd/=2;//taking the head bits into account
        // baseAdd+=512;//taking into account the first two megabytes
        Console::puts("ContframePool::get frame works beautifully. Req frames=");
        Console::putui(requestedFrameNum);
        Console::puts(" and the base=");
        Console::putui(baseAdd);
        Console::puts("\n");
        return baseAdd;
    }
    Console::puts("ContframePool::There is not enough frame for such a request with the size=");
    Console::putui(requestedFrameNum);
    Console::puts("\n");
    return -1; //reserved value for the situation that the getFrame cannot be fulfilled
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,unsigned long _n_frames){
    unsigned long baseFrameNum=_base_frame_no;
    unsigned long frameNum=_n_frames;
    unsigned long moreFrame=frameNum;
    unsigned long currentFrame=baseFrameNum;
    {//updating the head
        unsigned long charPage=int(baseFrameNum/8);
        unsigned long charRemainder=baseFrameNum%8;
        
        int dataBits[8];
        for (int bitCounter = 0 ; bitCounter != 8 ; bitCounter++) {
            dataBits[bitCounter] = (bitmap[charPage] & (1 << bitCounter)) != 0;
        }
        dataBits[charRemainder]=1;

        int tempVal=0;
        int pow=1;
        int counter=0;
        for(int i=0;i<8;i++){
            tempVal+=pow*dataBits[counter];
            pow*=2;
            counter++;
        }
        bitmap[charPage]=tempVal;
    }
    while(true){//updataing the avaialibility
        unsigned long charPage=int(currentFrame/8);
        unsigned long charRemainder=currentFrame%8;
        
        int dataBits[8];
        for (int bitCounter = 0 ; bitCounter != 8 ; bitCounter++) {
            dataBits[bitCounter] = (bitmap[charPage] & (1 << bitCounter)) != 0;
        }
        int tempEnd=charRemainder+2*moreFrame;
        if (tempEnd>8)
            tempEnd=8;
        for(int counter=charRemainder;counter<tempEnd;counter+=2){
            dataBits[counter]=0;
            moreFrame--;
            currentFrame+=2;
        }

        int tempVal=0;
            int pow=1;
            int counter=0;
            for(int i=0;i<8;i++){
                tempVal+=pow*dataBits[counter];
                pow*=2;
                counter++;
            }
            bitmap[charPage]=tempVal;
        if (moreFrame==0)
            break;
    }
    Console::puts("ContframePool::mark_inaccessible works beautifullu with the base=");
    Console::putui(baseFrameNum);
    Console::puts(" and frame number=");
    Console::putui(frameNum);
    Console::puts("\n");
}

void ContFramePool::release_frames(unsigned long _first_frame_no){
    ContFramePool* currentPool=NULL;
    for(int i=0;i<100;i++){
        if(pools[i]!=NULL){
            // Console::puts("base ");
            // Console::putui(pools[i]->baseFrameNum);
            // Console::puts("\n end");
            // Console::putui(pools[i]->baseFrameNum+pools[i]->numofFrame);
            // Console::puts("\n target");
            // Console::putui(_first_frame_no);
            // Console::puts("\n");
            if (pools[i]->baseFrameNum<=_first_frame_no && pools[i]->baseFrameNum+pools[i]->numofFrame>_first_frame_no){
                currentPool=pools[i];
                break;
            }
        }
    }

    if(!currentPool){
        Console::puts("ContframePool::release_frames error. MAYDAY");
        return;
    }
    unsigned char* currentBitmap=currentPool->bitmap;
    unsigned long firstHead=_first_frame_no*2+1;
    unsigned long secondHead=_first_frame_no+2;
    
    while(true){
        unsigned long charPage=int(secondHead/8);
        unsigned long charRemainder=secondHead%8;

        int dataBits[8];
        for (int bitCounter = 0 ; bitCounter != 8 ; bitCounter++) {
            dataBits[bitCounter] = (currentBitmap[charPage] & (1 << bitCounter)) != 0;
        }
        if(dataBits[charRemainder]==1)
            break;
        secondHead+=2;
        if(secondHead>=currentPool->numofFrame){
            Console::puts("ContframePool::release_frames unsuccessfull!\n");
            return;
        }
    }
    {//updating availibility
        unsigned long tempBegin=firstHead-1;
        unsigned long tempEnd=-1;
        unsigned long moreFrame=((secondHead-1)-(firstHead-1))/2; 
        while(true){
            int charPage=int(tempBegin/8);
            int charRemainder=tempBegin%8;
            int dataBits[8];
            for (int bitCounter = 0 ; bitCounter != 8 ; bitCounter++) {
                dataBits[bitCounter] = (currentBitmap[charPage] & (1 << bitCounter)) != 0;
            }
            if(charRemainder+2*moreFrame>=8)
                tempEnd=8;
            else
                tempEnd=charRemainder+2*moreFrame;
            for(int i=charRemainder;i<tempEnd;i+=2){
                dataBits[i]=1;
                moreFrame--;
                tempBegin+=2;
            }
            int tempVal=0;
            int pow=1;
            int counter=0;
            for(int i=0;i<8;i++){
                tempVal+=pow*dataBits[counter];
                pow*=2;
                counter++;
            }
            currentBitmap[charPage]=tempVal;
            if(moreFrame==0)
                break;
        }
    }
    {//updating head
        int charPage=int(firstHead/8);
        int charRemainder=firstHead%8;
        int dataBits[8];
        for (int bitCounter = 0 ; bitCounter != 8 ; bitCounter++) {
            dataBits[bitCounter] = (currentBitmap[charPage] & (1 << bitCounter)) != 0;
        }
        dataBits[charRemainder]=0;
        int tempVal=0;
        int pow=1;
        int counter=0;
        for(int i=0;i<8;i++){
            tempVal+=pow*dataBits[counter];
            pow*=2;
            counter++;
        }
        currentBitmap[charPage]=tempVal;
    }    
    Console::puts("ContframePool::release_frames is working beautifully.\n");
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames){
    unsigned long infoFrameNum=_n_frames;
    unsigned long neededInfoFrame;
    neededInfoFrame=int(infoFrameNum/FRAME_SIZE/4);
    if(infoFrameNum%(FRAME_SIZE*4)!=0)
        neededInfoFrame+=1;
    Console::puts("\nContframePool::need_info_frames is working beautifully. Neede info frame=");
    Console::putui(neededInfoFrame);
    Console::puts("\n");
    return neededInfoFrame;
}