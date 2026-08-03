# GNN与GCN基础笔记

## 1. 为什么需要GNN

CNN适合处理规则结构数据，例如图片：

    像素 像素 像素
    像素 像素 像素
    像素 像素 像素

像素具有固定邻居，因此可以使用固定卷积核。

但是很多现实数据是不规则结构：

-   社交网络
-   分子结构
-   CAD模型B-Rep拓扑结构

这些数据可以表示为图：

$$G=(V,E)$$

其中：

-   V：节点集合
-   E：边集合

## 2. GNN核心思想

GNN认为：

> 一个节点不仅包含自身信息，还应该融合邻居节点的信息。

例如CAD模型：

    Face2
     |
    Face1
     |
    Face3

Face1除了自身几何属性，还需要知道周围Face的信息。

## 3. Node Embedding（节点表示）

神经网络只能处理数字，因此需要将对象转换为向量。

例如一个CAD面：

    类型：圆柱面
    面积：100
    半径：5

表示为：

$$h=[100,5]$$

这个向量称为节点特征。

经过GNN消息传播后：

$$h'$$

包含：

-   自身属性
-   邻居属性
-   拓扑关系

称为Node Embedding。

## 4. Message Passing（消息传递）

GNN一层主要包含三个步骤：

    Message
     ↓
    Aggregate
     ↓
    Update

### 4.1 Message

节点向邻居发送信息：

$$m_{u\rightarrow v}=h_u$$

也可以考虑节点和边：

$$m_{u\rightarrow v}=M(h_u,h_v,e_{uv})$$

### 4.2 Aggregate

聚合邻居信息：

求和：

$$M_v=\sum_{u\in N(v)}m_u$$

平均：

$$M_v=\frac{1}{|N(v)|}\sum_{u\in N(v)}m_u$$

### 4.3 Update

更新节点：

$$h_v'=U(h_v,M_v)$$

完整形式：

$$h_v^{(k+1)}=U\!\left(h_v^{(k)},\operatorname{AGG}_{u\in N(v)}M\!\left(h_u^{(k)}\right)\right)$$

## 5. GNN与CNN关系

  CNN           GNN
  ------------- ----------------
  像素          节点
  卷积窗口      邻居集合
  卷积操作      消息传递
  Feature Map   Node Embedding
  感受野        k-hop邻域

二者本质都是：

> 聚合邻域信息。

# GCN（Graph Convolutional Network）

## 6. GCN是什么

GCN是GNN的一种。

它将Message Passing转换成矩阵运算。

核心思想：

$$\text{邻接矩阵}\times\text{节点特征}$$

即可完成邻居信息传播。

## 7. 邻接矩阵

邻接矩阵表示节点连接关系。

例如：

    A---B---C

对应：

$$
A=
\begin{bmatrix}
0 & 1 & 0\\
1 & 0 & 1\\
0 & 1 & 0
\end{bmatrix}
$$

计算：

$$AX$$

时：

每个节点会获得邻居节点的信息。

## 8. 为什么加入自环

GCN使用：

$$\hat{A}=A+I$$

原因：

普通邻接矩阵只包含邻居信息。

加入自身后：

节点可以同时获得：

-   自身特征
-   邻居特征

## 9. GCN公式

$$
H^{(l+1)}=\sigma\!\left(\hat{D}^{-\frac{1}{2}}\hat{A}\hat{D}^{-\frac{1}{2}}H^{(l)}W^{(l)}\right)
$$

## 10. 公式各项含义

### H：节点特征矩阵

表示所有节点的属性。

### A：邻接矩阵

表示节点之间的连接关系。

CAD中：

表示Face之间的拓扑邻接。

### D：度矩阵

度表示节点连接数量。

例如：

一个节点连接3个节点：

$$\deg(v)=3$$

度矩阵：

只在对角线上记录节点度。

## 11. 为什么需要归一化

如果直接：

$$AX$$

连接数量多的节点会产生更大的数值。

例如：

一个节点有100个邻居，另一个节点只有2个邻居。

简单求和会导致前者影响过大。

因此GCN使用：

$$
\hat{D}^{-\frac{1}{2}}\hat{A}\hat{D}^{-\frac{1}{2}}
$$

进行归一化。

作用：

让不同节点的信息传播更加平衡。

## 12. GCN与CAD的联系

B-Rep模型天然是图结构：

    Face
     |
    Edge
     |
    Face

节点：

Face

边：

拓扑邻接关系

节点特征：

-   面类型
-   面积
-   曲率
-   几何参数

传统方法：

人工规则：

    圆柱面 + 圆边 + 拓扑关系 = 孔

GNN/GCN：

自动学习：

    几何信息
    +
    拓扑关系

    ↓

    制造特征

# 总结

## GNN

核心：

节点通过Message Passing融合邻居信息，生成Node Embedding。

流程：

    节点特征
     ↓
    消息传递
     ↓
    邻居聚合
     ↓
    节点更新
     ↓
    Embedding

## GCN

核心：

使用归一化邻接矩阵实现Message Passing。

公式：

$$
H^{(l+1)}=\sigma\!\left(\hat{D}^{-\frac{1}{2}}\hat{A}\hat{D}^{-\frac{1}{2}}H^{(l)}W^{(l)}\right)
$$

理解：

-   A：决定信息从哪里传播
-   D：控制不同节点影响
-   H：节点特征
-   W：学习特征转换

本质：

GCN就是将GNN中的邻居信息传播过程矩阵化。
