/***********************************************************************
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF DALA, INC.
 * The copyright notice above does not evidence any actual or intended
 * publication of such source code.
 * (c) Copyright 2012-2025, DALA, All Rights Reserved.
 * http://www.dalacontroller.com/
 *
 * File: FIFO.h
 * Author: ouqingfeng
 * Description: FIFO buffer implementation
 * Created on: 2025.07.02
 *********************************************************************/
#ifndef _FIFO_H_
#define _FIFO_H_
//--------------------------------------文件包含---------------------------------------
#include "../../Libraries/ChipSupport/inc/BasicDataType.h"
//--------------------------------------常量定义---------------------------------------
//---------------------------------------全局宏-----------------------------------------
//-------------------------------------全局数据类型-------------------------------------
//---------------------------------------类定义-----------------------------------------
struct U8_FIFO_S
{
    uint8_t *pBUFF;
    uint16_t Head;
    uint16_t Tail;
    uint16_t Size;
    uint16_t MaxSize;
    float Load;
};
//-------------------------------------全局函数原型-------------------------------------
uint8_t U8_FIFO_Init(struct U8_FIFO_S *fifo, uint8_t *FIFOBuff, uint16_t Size);      // FIFO初始化
void U8_FIFO_Reset(struct U8_FIFO_S *fifo);                                          // FIFO复位
uint8_t U8_FIFO_Write(struct U8_FIFO_S *fifo, uint8_t Data);                         // 入队列
uint16_t U8_FIFO_Write_Batch(struct U8_FIFO_S *fifo, uint8_t *pData, uint16_t Size); // 写入队列大量数据
uint8_t U8_FIFO_Read(struct U8_FIFO_S *fifo, uint8_t *pData);                        // 出队列
uint16_t U8_FIFO_Read_Batch(struct U8_FIFO_S *fifo, uint8_t *Data, uint16_t Size);   // 出队列大量数据
uint16_t U8_FIFO_Size(struct U8_FIFO_S *fifo);                                       // 读取队列保存数据量FIFOsSize
float U8_FIFO_Load(struct U8_FIFO_S *fifo);
#endif /*FIFO_H_*/
