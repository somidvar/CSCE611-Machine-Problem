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

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)

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
int ContFramePool::PoolCounter=0;
ContFramePool* ContFramePool::Pools[] ={};

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    BaseAdd = _base_frame_no;//512 for kernel which is located at 2 MB
    FrameNum = _n_frames;//512 which translates into 2 MB located between 2MB and 4 MB
    InfoAdd = _info_frame_no;//Location of the manager
    InfoNum = _n_info_frames;//Number of frames required for the frame manager

    //To avoid fragmentation we should ensure that we have 4 page per char
    assert ((FrameNum % 4 ) == 0);
    
    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(InfoAdd == 0) {
        assert (FrameNum*FRAME_SIZE<=(2 MB));// to assure that it is maximum of 2 MB for Kernel frame pool
        assert (BaseAdd*FRAME_SIZE <= (4 MB)); //to assure the pool starts before 4 MB for kernel frame pool
        assert (BaseAdd*FRAME_SIZE >= (2 MB)); //to assure the pool ends after 4 MB for kernel frame pool

        Bitmap = (unsigned char *) (BaseAdd * FRAME_SIZE); // this is the frame manager for only the kernel
    } else {
        assert (FrameNum*FRAME_SIZE<=(28 MB));// to assure that it is maximum of 28 MB for proccess frame pool
        assert (BaseAdd*FRAME_SIZE <= (32 MB)); //to assure the pool starts before 32 MB for process frame pool
        assert (BaseAdd*FRAME_SIZE >= (4 MB)); //to assure the pool ends after 4 MB for process frame pool

        Bitmap = (unsigned char *) (InfoAdd * FRAME_SIZE); //this is the frame manager for only the process
    }
    
    //Initiliziation which makes all the bitmap 1 and all the head 0
    for(int i=0; i< FrameNum/4; i++) {
        FreeFrameNum+=4;
        Bitmap[i] = 85; //where the even bits are availibility and odd ones are the head
    }

    // Mark the first frame as being used if it is being used and as a head
    if(InfoAdd == 0) {//this is the kernel frame manager
        Bitmap[0] = 86;//making the first frame unavailaible as it holds kernel frame manager and 
        FreeFrameNum--;
    }

    //printinting the details of the pool
    Console::puts("Base=");Console::putui(BaseAdd);Console::puts(" Frame#=");Console::putui(FrameNum);
    Console::puts(" Info=");Console::putui(InfoAdd);Console::puts(" Info#=");Console::putui(InfoNum);
    Console::puts(" Free=");Console::putui(FreeFrameNum);Console::puts(" Pool#=");Console::putui(PoolCounter);
    Console::puts("\n");
    
    //keeping the track of the pools
    Pools[PoolCounter]= this;
    PoolCounter++;
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames){
    unsigned long requestedFrameNum = _n_frames;//number of frames needed for the process or kernel
    bool foundFlag=false;   
    unsigned long moreFrame;
    unsigned long bitCounter=0;
    unsigned long baseSeq=0;
    
    while(true){//finding a segment which can host the frames
        int charPage=int(bitCounter/8);
        int charRemainder=bitCounter%8;

        int dataBits[8];
        bitExt(Bitmap[charPage],dataBits);

        for(int j=charRemainder;j<8;j+=2){
            if(dataBits[j]==0){
                moreFrame=requestedFrameNum;
                baseSeq=bitCounter+2;
                break;
            }
            else{
                moreFrame--;
                if(moreFrame==0){
                    foundFlag=true;
                    break;
                }
            }
        }
        if(foundFlag)
            break;//successfully found a base address to host the data
        if(bitCounter==FrameNum)
            return 0;//unsuccessful operation,there is no fram available
        bitCounter+=2;
    }

    if(foundFlag){
        baseSeq=baseSeq/2;//the base addres for the sequence is the address of the bit rather than the frame which is fixed by dividing it by 2. Also, we have to add the base add of the pool
        baseSeq+=BaseAdd;
        mark_inaccessible(baseSeq,requestedFrameNum);//reserving the frames
        Console::puts("ContframePool::get_frames.");
        if(InfoAdd==0)
            Console::puts("KERNEL");
        else
            Console::puts("PROCESS");
        Console::puts(" need ");
        Console::putui(requestedFrameNum);
        Console::puts(" frames which starts at=");
        Console::putui(baseSeq);
        Console::puts("\n");
        return baseSeq;
    }
    Console::puts("ContframePool::There is not enough frame for such a request with the size=");
    Console::putui(requestedFrameNum);
    Console::puts("\n");
    return 0; //reserved value for the situation that the getFrame cannot be fulfilled
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,unsigned long _n_frames){
    unsigned long baseFrameNum=_base_frame_no;
    baseFrameNum-=BaseAdd;//removing the base address so it matches the bitmap
    unsigned long frameNum=_n_frames;
    unsigned long moreFrame=frameNum;
    unsigned long bitCounter=baseFrameNum*2;

    while(true){//updataing the avaialibility and heads
        unsigned long charPage=int(bitCounter/8);
        unsigned long charRemainder=bitCounter%8;
        
        int dataBits[8];
        bitExt(Bitmap[charPage],dataBits);

        int tempEnd=charRemainder+2*moreFrame;
        if (tempEnd>8)
            tempEnd=8;
        for(int counter=charRemainder;counter<tempEnd;counter+=2){
            dataBits[counter]=0;//making the bit unavailable
            dataBits[counter+1]=0;//removing previous heads
            moreFrame--;
            bitCounter+=2;
        }
        Bitmap[charPage]=decimalExt(dataBits);
        if (moreFrame==0)
            break;
    }
    bitCounter=baseFrameNum*2;
    {//updating the head
        unsigned long charPage=int(bitCounter/8);
        unsigned long charRemainder=bitCounter%8;
        
        int dataBits[8];
        bitExt(Bitmap[charPage],dataBits);
        dataBits[charRemainder+1]=1;//only one head is needed for the whole section
        Bitmap[charPage]=decimalExt(dataBits);
    }
    FreeFrameNum-=frameNum;
    Console::puts("ContframePool::mark_inaccessible, base=");
    Console::putui(baseFrameNum);
    Console::puts(" and frame number=");
    Console::putui(frameNum);
    Console::puts("\n");
}

void ContFramePool::release_frames(unsigned long _first_frame_no){
    unsigned long frameNum=_first_frame_no;
    ContFramePool* currentPool=NULL;
    for(int i=0;i<100;i++){
        if(Pools[i]!=NULL){
            if (Pools[i]->BaseAdd<=frameNum && Pools[i]->BaseAdd+Pools[i]->FrameNum>frameNum){
                currentPool=Pools[i];
                break;
            }
        }
    }

    if(!currentPool){
        Console::puts("ContframePool::release_frames could not locate the owner pool. MAYDAY\n");
        assert(false);
    }

    unsigned char* currentBitmap=currentPool->Bitmap;
    unsigned long firstHead=(frameNum-currentPool->BaseAdd)*2+1;
    unsigned long secondHead=firstHead+2;

    while(true){
        unsigned long charPage=int(secondHead/8);
        unsigned long charRemainder=secondHead%8;

        int dataBits[8];
        for (int i = 0 ; i != 8 ; i++) {
            dataBits[i] = (currentBitmap[charPage] & (1 << i)) != 0;
        }
        if(dataBits[charRemainder]==1)//next head is found
            break;
        if(dataBits[charRemainder-1]==1){//empty bit
            break;
        }   
        secondHead+=2;
        if(secondHead>=currentPool->FrameNum){
            Console::puts("ContframePool::headFiner-Warning, head is at end of pool!\n");
            assert(false);
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
                dataBits[i]=1;//updating the availibility bit
                dataBits[i+1]=0;//updating the head
                moreFrame--;
                tempBegin+=2;
            }
            int tempVal=0;
            int pow=1;
            for(int i=0;i<8;i++){
                tempVal+=pow*dataBits[i];
                pow*=2;
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
        for(int i=0;i<8;i++){
            tempVal+=pow*dataBits[i];
            pow*=2;
        }
        currentBitmap[charPage]=tempVal;
    
    }
    Console::puts("ContframePool::release_frames released from frame ");
    Console::putui(currentPool->BaseAdd+(firstHead-1)/2);
    Console::puts(" till ");
    Console::putui(currentPool->BaseAdd+(secondHead-1)/2);
    Console::puts("\n");
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames){
    unsigned long infoFrameNum=_n_frames;
    unsigned long neededInfoFrame=int(infoFrameNum/FRAME_SIZE/4);
    if(infoFrameNum%(FRAME_SIZE*4)!=0)
        neededInfoFrame+=1;
    Console::puts("ContframePool::need_info_frames is working beautifully. Neede info frame=");
    Console::putui(neededInfoFrame);
    Console::puts("\n");
    return neededInfoFrame;
}
void ContFramePool::bitExt(unsigned char input,int* dataBits){
    for (int i = 0 ; i != 8 ; i++) {
        dataBits[i] = (input & (1 << i)) != 0;
    }
}

int ContFramePool::decimalExt(int* dataBits){
    int tempVal=0;
    int pow=1;
    for(int i=0;i<8;i++){
        tempVal+=pow * dataBits[i];
        pow*=2;
    }
    return tempVal;
}