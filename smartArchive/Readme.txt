smartArchive程序文件说明

主要功能：
wyc C程序（方便使用Linux环境）
主控程序smartControl.c根据三种策略：
（1）Locality（只考虑数据本地性，将拥有最多本地数据的数据节点确定为编码节点）；
（2）Balance（只考虑负载均衡，将任务数最少的数据节点确定为编码节点）；
（3）First_Balance_Second_Locality（考虑数据本地性和负载均衡，第一关键字（任务数），第二关键字（数据本地性））；
确定编码节点以及编码参数（
编码结构参数（数据节点个数k、校验节点个数r）、
编码数据来源（编码的数据节点，即编码数据的提供者）、
编码数据去向（编码的校验节点，即编码数据的承受者）
），并触发集群完成编码过程


运行参数：

blocks.config			数据块存储位置元数据文件，格式：[数据节点编号]/t[数据块编号]
blocksRam.config		通过系统随机产生的数据块文件，格式：[数据节点编号]/t[数据块编号]
smartArchive.config		整个程序的配置文件，定义该程序中可以修改的常量
smartControl.c			主控程序，主要负责完成编码节点的选择
smartControl.h			smartControl.c的头文件，保存smartControl.c的配置常数
datanode.c				数据节点，其程序包括coding_node和data_node的代码
datanode.h				数据节点配置文件
