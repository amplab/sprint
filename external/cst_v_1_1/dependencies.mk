BitRank.o: BitRank.cpp BitRank.h ReplacePattern.h Tools.h
CHgtArray.o: CHgtArray.cpp CHgtArray.h CSA.h BitRank.h ReplacePattern.h \
  Tools.h wtreebwt.h bittree.h rbtree.h
CRMQ.o: CRMQ.cpp CRMQ.h SubblockRMQ.h Tools.h BitRank.h ReplacePattern.h
CSA.o: CSA.cpp CSA.h BitRank.h ReplacePattern.h Tools.h wtreebwt.h \
  bittree.h rbtree.h
Hash.o: Hash.cpp Hash.h Tools.h
LcpToParentheses.o: LcpToParentheses.cpp LcpToParentheses.h Tools.h \
  CHgtArray.h CSA.h BitRank.h ReplacePattern.h wtreebwt.h bittree.h \
  rbtree.h
Parentheses.o: Parentheses.cpp Parentheses.h Tools.h BitRank.h \
  ReplacePattern.h Hash.h
ReplacePattern.o: ReplacePattern.cpp ReplacePattern.h Tools.h
SSTree.o: SSTree.cpp SSTree.h SuffixTree.h Tools.h CSA.h BitRank.h \
  ReplacePattern.h wtreebwt.h bittree.h rbtree.h CHgtArray.h CRMQ.h \
  SubblockRMQ.h Parentheses.h Hash.h LcpToParentheses.h
SubblockRMQ.o: SubblockRMQ.cpp SubblockRMQ.h Tools.h BitRank.h \
  ReplacePattern.h
TestLCSS.o: TestLCSS.cpp SSTree.h SuffixTree.h Tools.h CSA.h BitRank.h \
  ReplacePattern.h wtreebwt.h bittree.h rbtree.h CHgtArray.h CRMQ.h \
  SubblockRMQ.h Parentheses.h Hash.h LcpToParentheses.h
Tools.o: Tools.cpp Tools.h
bittree.o: bittree.cpp bittree.h rbtree.h Tools.h
rbtree.o: rbtree.cpp rbtree.h
wtreebwt.o: wtreebwt.cpp wtreebwt.h bittree.h rbtree.h Tools.h
