//-------------------------------------------------------------------------------------------------
// 	可以自由使用，如果使用请注明出处和作者，如下
//	*******************************************************
//	File Path:	E:\GitHub\AIShare\src\TLibMain
//	File Name:	TAI_BevTree.cpp
// 	Author:		Finney
// 	Blog:		AI分享站(http://www.aisharing.com/)
// 	A-Email:	finneytang@gmail.com
//	*******************************************************
// 	Modify:		Eric 添加注释,说明
// 	E-EMail:	frederick.dang@gmail.com
// 	Git:		https://github.com/Eric-Dang/AIShare.git
//	*******************************************************
//	purpose:	
//-------------------------------------------------------------------------------------------------
#include "TAI_BevTree.h"

namespace TsiU{
	namespace AI{namespace BehaviorTree{
	//-------------------------------------------------------------------------------------
	// BevNodePrioritySelector
	//-------------------------------------------------------------------------------------
	// 遍历所有的子节点，检测是否有可以执行的节点，如果有则将可执行的节点的Index赋值给mui_CurrentSelectIndex	
	// 每次的评估都会重新检测所有的子节点
	bool BevNodePrioritySelector::_DoEvaluate(const BevNodeInputParam& input)
	{
		mui_CurrentSelectIndex = k_BLimited_InvalidChildNodeIndex;
		for(unsigned int i = 0; i < mul_ChildNodeCount; ++i)
		{
			BevNode* oBN = mao_ChildNodeList[i];
			if(oBN->Evaluate(input))
			{
				mui_CurrentSelectIndex = i;
				return true;
			}
		}
		return false;
	}
	void BevNodePrioritySelector::_DoTransition(const BevNodeInputParam& input)
	{
		if(_bCheckIndex(mui_LastSelectIndex))
		{
			BevNode* oBN = mao_ChildNodeList[mui_LastSelectIndex];
			oBN->Transition(input);
		}
		mui_LastSelectIndex = k_BLimited_InvalidChildNodeIndex;
	}
	BevRunningStatus BevNodePrioritySelector::_DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output)
	{
		BevRunningStatus bIsFinish = k_BRS_Finish;
		if(_bCheckIndex(mui_CurrentSelectIndex))
		{
			// 执行其他子节点时，执行上次节点的Transition
			// 执行完成后，修改上次执行的节点Index(mui_LastSelectIndex)
			if(mui_LastSelectIndex != mui_CurrentSelectIndex)  //new select result
			{
				if(_bCheckIndex(mui_LastSelectIndex))
				{
					BevNode* oBN = mao_ChildNodeList[mui_LastSelectIndex];
					oBN->Transition(input);   //we need transition
				}
				mui_LastSelectIndex = mui_CurrentSelectIndex;
			}
		}
		// 执行当前选着的子节点
		if(_bCheckIndex(mui_LastSelectIndex))
		{
			//Running node
			BevNode* oBN = mao_ChildNodeList[mui_LastSelectIndex];
			bIsFinish = oBN->Tick(input, output);
			//clear variable if finish
			if(bIsFinish)
				mui_LastSelectIndex = k_BLimited_InvalidChildNodeIndex;
		}
		return bIsFinish;
	}
	//-------------------------------------------------------------------------------------
	// BevNodeNonePrioritySelector
	//-------------------------------------------------------------------------------------
	// 和具有优先级的选择节点的区别，每次先检测当前的几点是否可以执行，如果可以则继续执行，如果不可以
	// 则执行BevNodePrioritySelector::_DoEvaluate重新选择
	bool BevNodeNonePrioritySelector::_DoEvaluate(const BevNodeInputParam& input)
	{
		if(_bCheckIndex(mui_CurrentSelectIndex))
		{
			BevNode* oBN = mao_ChildNodeList[mui_CurrentSelectIndex];
			if(oBN->Evaluate(input))
			{
				return true;
			}
		}
		return BevNodePrioritySelector::_DoEvaluate(input);
	}
	//-------------------------------------------------------------------------------------
	// BevNodeSequence
	//-------------------------------------------------------------------------------------
	// 每次只执行1个节点， 如果当前节点无效，即重头再来
	bool BevNodeSequence::_DoEvaluate(const BevNodeInputParam& input)
	{
		unsigned int testNode;
		if(mui_CurrentNodeIndex == k_BLimited_InvalidChildNodeIndex)
			testNode = 0;
		else
			testNode = mui_CurrentNodeIndex;

		if(_bCheckIndex(testNode))
		{
			BevNode* oBN = mao_ChildNodeList[testNode];
			if(oBN->Evaluate(input))
				return true;
		}
		return false;
	}
	void BevNodeSequence::_DoTransition(const BevNodeInputParam& input)
	{
		if(_bCheckIndex(mui_CurrentNodeIndex))
		{
			BevNode* oBN = mao_ChildNodeList[mui_CurrentNodeIndex];
			oBN->Transition(input);
		}
		mui_CurrentNodeIndex = k_BLimited_InvalidChildNodeIndex;
	}

	BevRunningStatus BevNodeSequence::_DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output)
	{
		BevRunningStatus bIsFinish = k_BRS_Finish;

		//First Time 循环执行
		if(mui_CurrentNodeIndex == k_BLimited_InvalidChildNodeIndex)
			mui_CurrentNodeIndex = 0;

		// 执行成功
		BevNode* oBN = mao_ChildNodeList[mui_CurrentNodeIndex];
		bIsFinish = oBN->Tick(input, output);
		// 子节点执行完成
		if(bIsFinish == k_BRS_Finish)
		{
			// 一个节点执行完成后，会将mui_CurrentNodeIndex执行下一个子节点的Index
			++mui_CurrentNodeIndex;
			//sequence is over 全部执行完成则将index设置成无效Index
			if(mui_CurrentNodeIndex == mul_ChildNodeCount)
			{
				mui_CurrentNodeIndex = k_BLimited_InvalidChildNodeIndex;
			}
			else
			{
				// 如果不是最后一个节点，并且当前子节点执行成功，则表明节点还在执行
				bIsFinish = k_BRS_Executing;
			}
		}
		// 执行报错 则执行无效
		if(bIsFinish < 0)
		{
			mui_CurrentNodeIndex = k_BLimited_InvalidChildNodeIndex;
		}
		return bIsFinish;
	}

	//-------------------------------------------------------------------------------------
	// BevNodeTerminal
	//-------------------------------------------------------------------------------------
	void BevNodeTerminal::_DoTransition(const BevNodeInputParam& input)
	{
		// mb_NeedExit = TRUE 属于异常退出
		if(mb_NeedExit)     //call Exit if we have called Enter 
			_DoExit(input, k_BRS_ERROR_Transition);

		SetActiveNode(NULL);
		me_Status = k_TNS_Ready;
		mb_NeedExit = false;
	}
	BevRunningStatus BevNodeTerminal::_DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output)
	{
		BevRunningStatus bIsFinish = k_BRS_Finish;
		
		if(me_Status == k_TNS_Ready)
		{
			_DoEnter(input);
			mb_NeedExit = TRUE;
			me_Status = k_TNS_Running;
			SetActiveNode(this);
		}
		if(me_Status == k_TNS_Running)
		{
			// TODO为什么这块每次都要调用 SetActiveNode(this);
			// 按照节点处理逻辑， k_TNS_Ready -> k_TNS_Running -> k_TNS_Finish -> k_TNS_Ready
			// 觉得这块不需要再SetActiveNode
			bIsFinish = _DoExecute(input, output);
			SetActiveNode(this);
			// TODO: 这块为什么要 || bIsFinish < 0 感觉这块没有必要 ？？
			if(bIsFinish == k_BRS_Finish || bIsFinish < 0)
				me_Status = k_TNS_Finish;
		}
		if(me_Status == k_TNS_Finish)
		{
			if(mb_NeedExit)     //call Exit if we have called Enter
				_DoExit(input, bIsFinish);

			me_Status = k_TNS_Ready;
			mb_NeedExit = FALSE;
			SetActiveNode(NULL);
			// 是否可以删除？？ TODO
			// return bIsFinish;
		}
		return bIsFinish;
	}

	//-------------------------------------------------------------------------------------
	// BevNodeParallel
	//-------------------------------------------------------------------------------------
	bool BevNodeParallel::_DoEvaluate(const BevNodeInputParam& input)
	{
		for(unsigned int i = 0; i < mul_ChildNodeCount; ++i)
		{
			BevNode* oBN = mao_ChildNodeList[i];
			if (mab_ChildNodeStatus[i] == k_BRS_Executing)
			{
				if(!oBN->Evaluate(input))
				{
					return false;
				}
			}
		}
		return true;
	}
	void BevNodeParallel::_DoTransition(const BevNodeInputParam& input)
	{
		for(unsigned int i = 0; i < k_BLimited_MaxChildNodeCnt; ++i)
			mab_ChildNodeStatus[i] = k_BRS_Executing;

		for(unsigned int i = 0; i < mul_ChildNodeCount; ++i)
		{
			BevNode* oBN = mao_ChildNodeList[i];
			oBN->Transition(input);
		}
	}
	BevNodeParallel& BevNodeParallel::SetFinishCondition(E_ParallelFinishCondition _e_Condition)
	{
		me_FinishCondition = _e_Condition;
		return (*this);
	}
	BevRunningStatus BevNodeParallel::_DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output)
	{   
		unsigned int finishedChildCount = 0;  
		for(unsigned int i = 0; i < mul_ChildNodeCount; ++i)
		{
			BevNode* oBN = mao_ChildNodeList[i];
			// k_PFC_OR 是指成功执行一个节点，还是执行完几个节点(是否成功无所谓)
			if(me_FinishCondition == k_PFC_OR)
			{
				if(mab_ChildNodeStatus[i] == k_BRS_Executing)
				{
					mab_ChildNodeStatus[i] = oBN->Tick(input, output);
				}

				// 节点需要执行成功吧！ TODO
				// 这块设计思路没有看明白， error也要返回finish吗？
				if(mab_ChildNodeStatus[i] != k_BRS_Executing)
				{
					for(unsigned int i = 0; i < k_BLimited_MaxChildNodeCnt; ++i)
						mab_ChildNodeStatus[i] = k_BRS_Executing;
					return k_BRS_Finish;
				}
			}
			else if(me_FinishCondition == k_PFC_AND)
			{
				if(mab_ChildNodeStatus[i] == k_BRS_Executing)
				{
					mab_ChildNodeStatus[i] = oBN->Tick(input, output);
				}
				if(mab_ChildNodeStatus[i] != k_BRS_Executing)
				{
					finishedChildCount++;
				}
			}
			else
			{
				D_CHECK(0);
;			}
		}
		if(finishedChildCount == mul_ChildNodeCount)
		{
			for(unsigned int i = 0; i < k_BLimited_MaxChildNodeCnt; ++i)
				mab_ChildNodeStatus[i] = k_BRS_Executing;
			return k_BRS_Finish;
		}
		return k_BRS_Executing;
	}
	//-------------------------------------------------------------------------------------
	// BevNodeLoop
	//-------------------------------------------------------------------------------------
	bool BevNodeLoop::_DoEvaluate(const BevNodeInputParam& input)
	{
		bool checkLoopCount = (mi_LoopCount == kInfiniteLoop) ||
			mi_CurrentCount < mi_LoopCount;

		if(!checkLoopCount)
			return false;

		if(_bCheckIndex(0))
		{
			BevNode* oBN = mao_ChildNodeList[0];
			if(oBN->Evaluate(input))
				return true;
		}
		return false;				 
	}
	void BevNodeLoop::_DoTransition(const BevNodeInputParam& input)
	{
		if(_bCheckIndex(0))
		{
			BevNode* oBN = mao_ChildNodeList[0];
			oBN->Transition(input);
		}
		mi_CurrentCount = 0;
	}
	BevRunningStatus BevNodeLoop::_DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output)
	{
		BevRunningStatus bIsFinish = k_BRS_Finish;
		if(_bCheckIndex(0))
		{
			BevNode* oBN = mao_ChildNodeList[0];
			bIsFinish = oBN->Tick(input, output);

			if(bIsFinish == k_BRS_Finish)
			{
				if(mi_LoopCount != kInfiniteLoop)
				{
					mi_CurrentCount++;
					// TODO: 这块判断条件应该是 mi_CurrentCount != mi_LoopCount, 否则就只执行1次
					if(mi_CurrentCount == mi_LoopCount)
					{
						bIsFinish = k_BRS_Executing;
					}
				}
				else
				{
					bIsFinish = k_BRS_Executing;
				}
			}
		}
		if(bIsFinish)
		{
			mi_CurrentCount = 0;
		}
		return bIsFinish;
	}	
}}}