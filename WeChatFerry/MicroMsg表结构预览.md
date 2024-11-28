# MicroMsg数据库结构说明

## **主要表**

### **1. Contact (联系人)**
| 字段名称             | 类型      | 说明           |
|----------------------|-----------|----------------|
| UserName            | TEXT      | 用户名         |
| Alias               | TEXT      | 别名           |
| NickName            | TEXT      | 昵称           |
| DelFlag             | INTEGER   | 删除标志       |
| VerifyFlag          | INTEGER   | 验证标志       |
| Remark              | TEXT      | 备注           |
| LabelIDList         | TEXT      | 标签 ID 列表   |
| DomainList          | TEXT      | 域名列表       |
| ChatRoomType        | INT       | 群类型         |
| HeadImgMd5          | TEXT      | 头像 MD5       |

**用途**: 记录联系人信息，包括昵称、别名、头像等。

---

### **2. ChatRoom (群信息)**
| 字段名称             | 类型      | 说明           |
|----------------------|-----------|----------------|
| ChatRoomName        | TEXT      | 群名称         |
| UserNameList        | TEXT      | 用户名列表     |
| DisplayNameList     | TEXT      | 显示名称列表   |
| RoomData            | BLOB      | 群数据         |

**用途**: 存储群的基本信息，包括成员列表和显示名称等。

---

### **3. ChatRoomInfo (群详细信息)**
| 字段名称             | 类型      | 说明           |
|----------------------|-----------|----------------|
| ChatRoomName        | TEXT      | 群名称         |
| Announcement        | TEXT      | 群公告         |
| AnnouncementEditor  | TEXT      | 公告编辑者     |
| AnnouncementPublishTime | INTEGER | 公告发布时间   |
| ChatRoomStatus      | INTEGER   | 群状态         |

**用途**: 存储与群相关的详细信息，例如公告内容和群状态。

---

### **4. Session (会话)**
| 字段名称             | 类型      | 说明           |
|----------------------|-----------|----------------|
| strUsrName          | TEXT      | 用户名         |
| nOrder              | INT       | 排序号         |
| nUnReadCount        | INTEGER   | 未读消息数     |
| strNickName         | TEXT      | 昵称           |
| strContent          | TEXT      | 消息内容       |
| nTime               | INTEGER   | 消息时间戳     |

**用途**: 记录会话信息，包括用户的未读消息数、最后一条消息内容及其时间。

---

### **5. AppInfo (应用信息)**
| 字段名称             | 类型      | 说明           |
|----------------------|-----------|----------------|
| AppId               | TEXT      | 应用 ID        |
| Name                | TEXT      | 应用名称       |
| Description         | TEXT      | 应用描述       |
| Version             | INT       | 应用版本       |
| IconUrl             | TEXT      | 图标地址       |

**用途**: 存储与应用程序相关的元信息。

---

## **辅助表**

### **6. ContactHeadImgUrl (联系人头像信息)**
| 字段名称             | 类型      | 说明           |
|----------------------|-----------|----------------|
| usrName             | TEXT      | 用户名         |
| smallHeadImgUrl     | TEXT      | 小头像 URL     |
| bigHeadImgUrl       | TEXT      | 大头像 URL     |

**用途**: 用于存储联系人的头像信息。

---

### **7. BizInfo (业务信息)**
| 字段名称             | 类型      | 说明           |
|----------------------|-----------|----------------|
| UserName            | TEXT      | 用户名         |
| Type                | INTEGER   | 类型           |
| BrandList           | TEXT      | 品牌列表       |
| BrandInfo           | TEXT      | 品牌信息       |
| BrandIconURL        | TEXT      | 品牌图标 URL   |

**用途**: 存储与企业账户或品牌相关的信息。

---

### **8. TicketInfo (票据信息)**
| 字段名称             | 类型      | 说明           |
|----------------------|-----------|----------------|
| UserName            | TEXT      | 用户名         |
| Ticket              | TEXT      | 票据           |

**用途**: 存储用户票据信息，可能用于登录或认证。

---

### **9. RevokeMsgStorage (撤回消息存储)**
| 字段名称             | 类型      | 说明           |
|----------------------|-----------|----------------|
| CreateTime          | INTEGER   | 创建时间       |
| MsgSvrID            | INTEGER   | 消息服务 ID    |
| RevokeSvrID         | INTEGER   | 撤回服务 ID    |

**用途**: 记录已撤回的消息。

---

## **其他信息表**

| 表名称                | 说明                                           |
|-----------------------|----------------------------------------------|
| **DelayDownLoad**     | 延迟下载消息记录。                             |
| **FTSContactTrans**   | 索引或检索相关表，用于优化搜索。               |
| **FTSChatroomTrans**  | 索引或检索相关表，用于优化搜索。               |
| **BizName2ID**        | 企业相关的订阅信息。                           |
| **BizSessionNewFeeds**| 企业相关的新会话信息。                         |

---

