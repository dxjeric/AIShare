//-------------------------------------------------------------------------------------------------
// 	可以自由使用，如果使用请注明出处和作者，如下
//	*******************************************************
//	file path:	E:\GitHub\AIShare\src\TLibMain
//	file name:	TAI_BevTree.h
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
#ifndef __TAI_BEVTREE_H__
#define __TAI_BEVTREE_H__

#include <string>
#include "TUtility_AnyData.h"

namespace TsiU{
	namespace AI{namespace BehaviorTree{
// 自己点的最大数量 因为子节点存储使用的是数组，Index是0-(k_BLimited_MaxChildNodeCnt-1)，因此k_BLimited_MaxChildNodeCnt也是无效的Index 
#define k_BLimited_MaxChildNodeCnt              16		
#define k_BLimited_InvalidChildNodeIndex        k_BLimited_MaxChildNodeCnt

		enum E_ParallelFinishCondition
		{
			k_PFC_OR = 1,
			k_PFC_AND
		};

		enum BevRunningStatus
		{
			k_BRS_Executing					= 0,
			k_BRS_Finish					= 1,
			k_BRS_ERROR_Transition			= -1,
		};

		enum E_TerminalNodeStaus
		{
			k_TNS_Ready         = 1,
			k_TNS_Running       = 2,
			k_TNS_Finish        = 3,
		};

		typedef AnyData BevNodeInputParam;
		typedef AnyData BevNodeOutputParam;

		//-------------------------------------------------------------------------------------------------------------------------------------
		// 节点预判类
		class BevNodePrecondition
		{
		public:
			virtual bool ExternalCondition(const BevNodeInputParam& input) const = 0;
		};
		class BevNodePreconditionTRUE : public BevNodePrecondition
		{
		public:
			virtual bool ExternalCondition(const BevNodeInputParam& input) const{
				return true;
			}
		};
		class BevNodePreconditionFALSE : public BevNodePrecondition
		{
		public:
			virtual bool ExternalCondition(const BevNodeInputParam& input) const{
				return false;
			}
		};
		class BevNodePreconditionNOT : public BevNodePrecondition
		{
		public:
			BevNodePreconditionNOT(BevNodePrecondition* lhs)
				: m_lhs(lhs)
			{
				D_CHECK(m_lhs);
			}
			~BevNodePreconditionNOT(){
				D_SafeDelete(m_lhs);
			}
			virtual bool ExternalCondition(const BevNodeInputParam& input) const{
				return !m_lhs->ExternalCondition(input);
			}
		private:
			BevNodePrecondition* m_lhs;
		};
		class BevNodePreconditionAND : public BevNodePrecondition
		{
		public:
			BevNodePreconditionAND(BevNodePrecondition* lhs, BevNodePrecondition* rhs)
				: m_lhs(lhs)
				, m_rhs(rhs)
			{
				D_CHECK(m_lhs && m_rhs);
			}
			~BevNodePreconditionAND(){
				D_SafeDelete(m_lhs);
				D_SafeDelete(m_rhs);
			}
			virtual bool ExternalCondition(const BevNodeInputParam& input) const{
				return m_lhs->ExternalCondition(input) && m_rhs->ExternalCondition(input);
			}
		private:
			BevNodePrecondition* m_lhs;
			BevNodePrecondition* m_rhs;
		};
		class BevNodePreconditionOR : public BevNodePrecondition
		{
		public:
			BevNodePreconditionOR(BevNodePrecondition* lhs, BevNodePrecondition* rhs)
				: m_lhs(lhs)
				, m_rhs(rhs)
			{
				D_CHECK(m_lhs && m_rhs);
			}
			~BevNodePreconditionOR(){
				D_SafeDelete(m_lhs);
				D_SafeDelete(m_rhs);
			}
			virtual bool ExternalCondition(const BevNodeInputParam& input) const{
				return m_lhs->ExternalCondition(input) || m_rhs->ExternalCondition(input);
			}
		private:
			BevNodePrecondition* m_lhs;
			BevNodePrecondition* m_rhs;
		};
		class BevNodePreconditionXOR : public BevNodePrecondition
		{
		public:
			BevNodePreconditionXOR(BevNodePrecondition* lhs, BevNodePrecondition* rhs)
				: m_lhs(lhs)
				, m_rhs(rhs)
			{
				D_CHECK(m_lhs && m_rhs);
			}
			~BevNodePreconditionXOR(){
				D_SafeDelete(m_lhs);
				D_SafeDelete(m_rhs);
			}
			virtual bool ExternalCondition(const BevNodeInputParam& input) const{
				return m_lhs->ExternalCondition(input) ^ m_rhs->ExternalCondition(input);
			}
		private:
			BevNodePrecondition* m_lhs;
			BevNodePrecondition* m_rhs;
		};
		//-------------------------------------------------------------------------------------------------------------------------------------
		// 行为树节点父类
		class BevNode
		{
		public:
			BevNode(BevNode* _o_ParentNode, BevNodePrecondition* _o_NodeScript = NULL)
				: mul_ChildNodeCount(0)
				, mz_DebugName("UNNAMED")
				, mo_ActiveNode(NULL)
				, mo_LastActiveNode(NULL)
				, mo_NodePrecondition(NULL)
			{
				for(int i = 0; i < k_BLimited_MaxChildNodeCnt; ++i)
					mao_ChildNodeList[i] = NULL;

				_SetParentNode(_o_ParentNode);
				SetNodePrecondition(_o_NodeScript);
			}
			virtual ~BevNode()
			{
				for(unsigned int i = 0; i < mul_ChildNodeCount; ++i)
				{
					D_SafeDelete(mao_ChildNodeList[i]);
				}
				D_SafeDelete(mo_NodePrecondition);
			}
			// 检查节点是否可以执行， 验证了前置条件以及节点自身的评估
			bool Evaluate(const BevNodeInputParam& input)
			{
				return (mo_NodePrecondition == NULL || mo_NodePrecondition->ExternalCondition(input)) && _DoEvaluate(input);
			}
			// TODO: _DoTransition 遗留，后续添加注释
			// 选择节点在执行完一个子节点后，切换到其他节点时，需要先执行
			void Transition(const BevNodeInputParam& input)
			{
				_DoTransition(input);
			}
			BevRunningStatus Tick(const BevNodeInputParam& input, BevNodeOutputParam& output)
			{
				return _DoTick(input, output);
			}
			//---------------------------------------------------------------
			// 添加子节点
			BevNode& AddChildNode(BevNode* _o_ChildNode)
			{
				if(mul_ChildNodeCount == k_BLimited_MaxChildNodeCnt)
				{
					D_Output("The number of child nodes is up to 16");
					D_CHECK(0);
					return (*this);
				}
				mao_ChildNodeList[mul_ChildNodeCount] = _o_ChildNode;
				++mul_ChildNodeCount;
				return (*this);
			}
			// 设置节点前置条件， 更换条件
			BevNode& SetNodePrecondition(BevNodePrecondition* _o_NodePrecondition)
			{
				if(mo_NodePrecondition != _o_NodePrecondition)
				{
					if(mo_NodePrecondition)
						delete mo_NodePrecondition;

					mo_NodePrecondition = _o_NodePrecondition;
				}
				return (*this);
			}
			// 设置节点标示
			BevNode& SetDebugName(const char* _debugName)
			{
				mz_DebugName = _debugName;
				return (*this);
			}
			// TODO:获取上次激活的节点
			const BevNode* oGetLastActiveNode() const
			{
				return mo_LastActiveNode;
			}
			// 设置当前被激活的节点
			// 1. 设置上次执行的节点
			// 2. 设置当前激活的节点
			// 3. 同时设置父节点的的执行节点， 最后Root节点保存了当前执行的节点，下次就不在需要遍历 TODO
			void SetActiveNode(BevNode* _o_Node)
			{
				mo_LastActiveNode = mo_ActiveNode;
				mo_ActiveNode = _o_Node;
				if(mo_ParentNode != NULL)
					mo_ParentNode->SetActiveNode(_o_Node);
			}
			// 获取节点标示
			const char* GetDebugName() const
			{
				return mz_DebugName.c_str();
			}
		protected:
			//--------------------------------------------------------------
			// virtual function
			//--------------------------------------------------------------
			// 评估是否需要执行该节点
			virtual bool _DoEvaluate(const BevNodeInputParam& input)
			{
				return true;
			}

			// TODO: 在变换到其他节点前，需要执行的动作， 也可称为离开节点需要执行的动作
			virtual void _DoTransition(const BevNodeInputParam& input)
			{
			}
			// 节点循环执行的入口
			virtual BevRunningStatus _DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output)
			{
				return k_BRS_Finish;
			}
		protected:
			// 设置父节点
			void _SetParentNode(BevNode* _o_ParentNode)
			{
				mo_ParentNode = _o_ParentNode;
			}
			// 检查Index的有效性
			bool _bCheckIndex(u32 _ui_Index) const
			{
				return _ui_Index >= 0 && _ui_Index < mul_ChildNodeCount;
			}
		protected:

			// 子节点
			BevNode*                mao_ChildNodeList[k_BLimited_MaxChildNodeCnt];

			// 子节点数量
			u32						mul_ChildNodeCount;

			// 父节点
			BevNode*                mo_ParentNode;

			// TODO: 当前激活的节点，目的是减少遍历
			BevNode*                mo_ActiveNode;

			// TODO: 上次激活的节点,暂时还不知道什么作用
			BevNode*				mo_LastActiveNode;

			// 节点执行的预判条件
			BevNodePrecondition*    mo_NodePrecondition;

			// 节点标示符
			std::string				mz_DebugName;
		};

		// 具有优先级的selector节点， 如何体现这个优先级
		class BevNodePrioritySelector : public BevNode
		{
		public:
			BevNodePrioritySelector(BevNode* _o_ParentNode, BevNodePrecondition* _o_NodePrecondition = NULL)
				: BevNode(_o_ParentNode, _o_NodePrecondition)
				, mui_LastSelectIndex(k_BLimited_InvalidChildNodeIndex)
				, mui_CurrentSelectIndex(k_BLimited_InvalidChildNodeIndex)
			{}

			// 遍历所有的子节点，检测是否有可以执行的节点, mui_CurrentSelectIndex将要执行的节点Index
			virtual bool _DoEvaluate(const BevNodeInputParam& input);
			// TODO: 在变换到其他节点前，需要执行的动作， 也可称为离开节点需要执行的动作
			virtual void _DoTransition(const BevNodeInputParam& input);
			// 执行接口
			virtual BevRunningStatus _DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output);

		protected:
			// 当前Tick将要执行的节点Index
			u32 mui_CurrentSelectIndex;
			// 上次执行的节点Index
			u32 mui_LastSelectIndex;
		};

		// 没有优先级的选择节点		
		class BevNodeNonePrioritySelector : public BevNodePrioritySelector
		{
		public:
			BevNodeNonePrioritySelector(BevNode* _o_ParentNode, BevNodePrecondition* _o_NodePrecondition = NULL)
				: BevNodePrioritySelector(_o_ParentNode, _o_NodePrecondition)
			{}
			virtual bool _DoEvaluate(const BevNodeInputParam& input);
		};

		// 顺序节点
		class BevNodeSequence : public BevNode
		{
		public:
			BevNodeSequence(BevNode* _o_ParentNode, BevNodePrecondition* _o_NodePrecondition = NULL)
				: BevNode(_o_ParentNode, _o_NodePrecondition)
				, mui_CurrentNodeIndex(k_BLimited_InvalidChildNodeIndex)
			{}
			virtual bool _DoEvaluate(const BevNodeInputParam& input);
			virtual void _DoTransition(const BevNodeInputParam& input);
			virtual BevRunningStatus _DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output);

		private:
			// 当前执行到的节点的Index
			u32 mui_CurrentNodeIndex;
		};

		// TODO:终端节点 作用？？？
		// 含有简单的状态机
		class BevNodeTerminal : public BevNode
		{
		public:
			BevNodeTerminal(BevNode* _o_ParentNode, BevNodePrecondition* _o_NodePrecondition = NULL)
				: BevNode(_o_ParentNode, _o_NodePrecondition)
				, me_Status(k_TNS_Ready)
				, mb_NeedExit(false)
			{}
			virtual void _DoTransition(const BevNodeInputParam& input);
			// 根据当前节点的状态执行对应的处理
			// k_TNS_Ready:		设置当前激活节点， 跳转状态到Running, 后续继续执行Running操作
			// k_TNS_Running:	执行_DoExecute,根据返回再设定后续状态, 如果设置为TNS_Finish，则会后续直接执行Finish操作
			// k_TNS_Finish:	执行Finish操作， 并且清空节点相关数据
			virtual BevRunningStatus _DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output);

		protected:
			// _DoEnter 和 _DoExit 必须配对出现， 如果有Enter在节点执行完成前必须有Exit, 否则都不出现
			// Init
			virtual void				_DoEnter(const BevNodeInputParam& input)								{}
			// Runing 执行
			virtual BevRunningStatus	_DoExecute(const BevNodeInputParam& input, BevNodeOutputParam& output)	{ return k_BRS_Finish;}
			// Leave
			virtual void				_DoExit(const BevNodeInputParam& input, BevRunningStatus _ui_ExitID)	{}

		private:
			E_TerminalNodeStaus me_Status;
			bool                mb_NeedExit;
		};

		// 并发节点
		class BevNodeParallel : public BevNode
		{
		public:
			BevNodeParallel(BevNode* _o_ParentNode, BevNodePrecondition* _o_NodePrecondition = NULL)
				: BevNode(_o_ParentNode, _o_NodePrecondition)
				, me_FinishCondition(k_PFC_OR)
			{
				for(unsigned int i = 0; i < k_BLimited_MaxChildNodeCnt; ++i)
					mab_ChildNodeStatus[i] = k_BRS_Executing;
			}
			// 检查所有需要执行的节点是否都可以执行
			virtual bool _DoEvaluate(const BevNodeInputParam& input);
			// 重新设置所有子节点可运行， 同时执行所有子节点的Transition
			virtual void _DoTransition(const BevNodeInputParam& input);
			virtual BevRunningStatus _DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output);

			BevNodeParallel& SetFinishCondition(E_ParallelFinishCondition _e_Condition);

		private:
			// 返回结果处理方式 And or Or
			E_ParallelFinishCondition me_FinishCondition;
			// 子节点的运行状态
			BevRunningStatus		  mab_ChildNodeStatus[k_BLimited_MaxChildNodeCnt];
		};

		// 循环节点 该节点只允许有一个子节点
		class BevNodeLoop : public BevNode
		{
		public:
			static const int kInfiniteLoop = -1;

		public:
			BevNodeLoop(BevNode* _o_ParentNode, BevNodePrecondition* _o_NodePrecondition = NULL, int _i_LoopCnt = kInfiniteLoop)
				: BevNode(_o_ParentNode, _o_NodePrecondition)
				, mi_LoopCount(_i_LoopCnt)
				, mi_CurrentCount(0)
			{}
			virtual bool _DoEvaluate(const BevNodeInputParam& input);
			virtual void _DoTransition(const BevNodeInputParam& input);
			virtual BevRunningStatus _DoTick(const BevNodeInputParam& input, BevNodeOutputParam& output);

		private:
			// 循环的次数
			int mi_LoopCount;
			// 当前已经执行的次数
			int mi_CurrentCount;
		};

		class BevNodeFactory
		{
		public:
			static BevNode& oCreateParallelNode(BevNode* _o_Parent, E_ParallelFinishCondition _e_Condition, const char* _debugName)
			{
				BevNodeParallel* pReturn = new BevNodeParallel(_o_Parent);
				pReturn->SetFinishCondition(_e_Condition);
				oCreateNodeCommon(pReturn, _o_Parent, _debugName);
				return (*pReturn);
			}
			static BevNode& oCreatePrioritySelectorNode(BevNode* _o_Parent, const char* _debugName)
			{
				BevNodePrioritySelector* pReturn = new BevNodePrioritySelector(_o_Parent);
				oCreateNodeCommon(pReturn, _o_Parent, _debugName);
				return (*pReturn);
			}
			static BevNode& oCreateNonePrioritySelectorNode(BevNode* _o_Parent, const char* _debugName)
			{
				BevNodeNonePrioritySelector* pReturn = new BevNodeNonePrioritySelector(_o_Parent);
				oCreateNodeCommon(pReturn, _o_Parent, _debugName);
				return (*pReturn);
			}
			static BevNode& oCreateSequenceNode(BevNode* _o_Parent, const char* _debugName)
			{
				BevNodeSequence* pReturn = new BevNodeSequence(_o_Parent);
				oCreateNodeCommon(pReturn, _o_Parent, _debugName);
				return (*pReturn);
			}
			static BevNode& oCreateLoopNode(BevNode* _o_Parent, const char* _debugName, int _i_LoopCount)
			{
				BevNodeLoop* pReturn = new BevNodeLoop(_o_Parent, NULL, _i_LoopCount);
				oCreateNodeCommon(pReturn, _o_Parent, _debugName);
				return (*pReturn);
			}
			// 创建叶子节点 class T : public BevNodeTerminal
			template<typename T>
			static BevNode& oCreateTeminalNode(BevNode* _o_Parent, const char* _debugName)
			{
				BevNodeTerminal* pReturn = new T(_o_Parent);
				oCreateNodeCommon(pReturn, _o_Parent, _debugName);
				return (*pReturn);
			}
		private:
			static void oCreateNodeCommon(BevNode* _o_Me, BevNode* _o_Parent, const char* _debugName)
			{
				if(_o_Parent)
					_o_Parent->AddChildNode(_o_Me);
				_o_Me->SetDebugName(_debugName);
			}
		};
	}}}

#endif
