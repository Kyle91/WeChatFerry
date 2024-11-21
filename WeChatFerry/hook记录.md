1. hook实时消息的思路
https://www.52pojie.cn/forum.php?mod=viewthread&tid=1970822&highlight=%CE%A2%D0%C5%CF%FB%CF%A2

2. 邀请进群，可以尝试用ghidra、IDA搜索@im.chatroom 、 NetSceneInviteChatRoomMember

3. 添加成员到群，尝试用ghidra、IDA搜索 ChatRoomMgr::doAddMemberToChatRoom

4. 获取登录二维码，ghidra搜索 NetSceneCheckLoginQRCode::NetSceneCheckLoginQRCode，，有一处这样的地方 
     v13 = sub_182022E80();
	 sub_1823B7730(v12, v13 + 16);   hook这个sub_1823B7730，里面的a2参数就可以拿到二维码，读取22个字节即可，拿到的是一个字符串，再拼上http://weixin.qq.com/x/
	 
5. 群成员新增通知，用ghidra、IDA搜索ChatRoomMgr::updateChatRoomMemberDetail，可以搜到sub_182162bc0,这里是群成员信息变动的通知，其中a4+32就是变动的成员数量，在方法中往下找可以找到
	&micromsg::ChatRoomMemberInfo::`vftable'; 这里就是准备群成员的结构体，sub_1834F5730((__int64)&v167, v60);这个方法就是给数据赋值，通过查看sub_1834F5730，里面有变动成员的wxid，昵称，邀请人
	那我们就可以hook sub_182162bc0拿到群ID，变动的成员信息。要注意的是这里只要成员信息变动就会调用，所以还得结合入群的系统消息来判断，确定哪一个是新增的成员
	 
手动编译proto,在tool目录
protoc --proto_path=..\proto --nanopb_out=. --experimental_allow_proto3_optional ..\proto\wcf.proto


