1. hook实时消息的思路
https://www.52pojie.cn/forum.php?mod=viewthread&tid=1970822&highlight=%CE%A2%D0%C5%CF%FB%CF%A2

2. 邀请进群，可以尝试用IDA搜索@im.chatroom 、 NetSceneInviteChatRoomMember

3. 添加成员到群，尝试用IDA搜索 ChatRoomMgr::doAddMemberToChatRoom

4. 获取登录二维码，ghidra搜索 NetSceneCheckLoginQRCode::NetSceneCheckLoginQRCode，，有一处这样的地方 
     v13 = sub_182022E80();
	 sub_1823B7730(v12, v13 + 16);   hook这个sub_1823B7730，里面的a2参数就可以拿到二维码，读取22个字节即可，拿到的是一个字符串，再拼上http://weixin.qq.com/x/