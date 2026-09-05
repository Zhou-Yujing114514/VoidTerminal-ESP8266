#include "chat.h"
#include "wifi_config.h"

// 服务器证书 SHA1 指纹（二进制 20 字节），供 beginSSL 与 setFingerprint 使用
static const uint8_t chatSslFingerprint[20] = {
    0x1C,0x86,0x71,0xD8,0xC7,0x8C,0xC4,0xBA,0x58,0x43,
    0xB6,0x12,0xFF,0x36,0x4E,0x63,0x7E,0x51,0xFA,0xE1
};
#include <pgmspace.h>

ChatManager chat;

const char gridLetters_0[] PROGMEM = ".,!?";
const char gridLetters_1[] PROGMEM = "abc";
const char gridLetters_2[] PROGMEM = "def";
const char gridLetters_3[] PROGMEM = "ghi";
const char gridLetters_4[] PROGMEM = "jkl";
const char gridLetters_5[] PROGMEM = "mno";
const char gridLetters_6[] PROGMEM = "pqrs";
const char gridLetters_7[] PROGMEM = "tuv";
const char gridLetters_8[] PROGMEM = "wxyz";

const char* const gridLetters[9] PROGMEM = {
    gridLetters_0, gridLetters_1, gridLetters_2,
    gridLetters_3, gridLetters_4, gridLetters_5,
    gridLetters_6, gridLetters_7, gridLetters_8
};

struct PinyinCandidate {
    const char* pinyin;
    const char* candidates[8];
};

// 全拼单字词库
const PinyinCandidate candidateTable[] PROGMEM = {
    {"a", {"啊", "阿", "呵", "嗄", "腌", "锕", "啊", "阿"}},
    {"ai", {"爱", "哎", "唉", "矮", "艾", "挨", "癌", "蔼"}},
    {"an", {"安", "按", "暗", "岸", "案", "俺", "铵", "胺"}},
    {"ang", {"昂", "盎", "肮", "昂", "盎", "肮", "昂", "盎"}},
    {"ao", {"奥", "袄", "傲", "凹", "敖", "翱", "澳", "懊"}},
    {"ba", {"吧", "把", "八", "爸", "霸", "罢", "巴", "拔"}},
    {"bai", {"白", "百", "拜", "败", "摆", "佰", "柏", "稗"}},
    {"ban", {"半", "办", "班", "版", "般", "板", "伴", "搬"}},
    {"bang", {"帮", "棒", "榜", "绑", "磅", "蚌", "谤", "梆"}},
    {"bao", {"包", "报", "保", "抱", "爆", "宝", "薄", "暴"}},
    {"bei", {"被", "北", "杯", "背", "悲", "贝", "备", "卑"}},
    {"ben", {"本", "笨", "奔", "苯", "畚", "夯", "本", "笨"}},
    {"beng", {"崩", "绷", "甭", "蹦", "迸", "绷", "崩", "蹦"}},
    {"bi", {"比", "笔", "必", "币", "毕", "闭", "壁", "臂"}},
    {"bian", {"变", "边", "便", "遍", "编", "辩", "辨", "辫"}},
    {"biao", {"表", "标", "彪", "膘", "镖", "飙", "裱", "鳔"}},
    {"bie", {"别", "憋", "瘪", "鳖", "蹩", "别", "憋", "瘪"}},
    {"bin", {"宾", "滨", "彬", "斌", "濒", "殡", "髌", "鬓"}},
    {"bing", {"并", "病", "冰", "兵", "饼", "炳", "柄", "禀"}},
    {"bo", {"不", "波", "博", "播", "驳", "泊", "勃", "铂"}},
    {"bu", {"不", "步", "部", "布", "怖", "捕", "哺", "埠"}},
    {"ca", {"擦", "嚓", "礤", "擦", "嚓", "擦", "嚓", "擦"}},
    {"cai", {"才", "菜", "财", "猜", "材", "裁", "采", "彩"}},
    {"can", {"参", "餐", "残", "惨", "蚕", "灿", "璨", "孱"}},
    {"cang", {"藏", "仓", "苍", "舱", "沧", "伧", "藏", "仓"}},
    {"cao", {"草", "操", "曹", "槽", "糙", "嘈", "漕", "艚"}},
    {"ce", {"册", "测", "侧", "厕", "策", "恻", "岑", "层"}},
    {"ceng", {"层", "曾", "蹭", "层", "曾", "蹭", "层", "曾"}},
    {"cha", {"查", "茶", "差", "插", "察", "叉", "茬", "碴"}},
    {"chai", {"拆", "柴", "差", "豺", "侪", "虿", "拆", "柴"}},
    {"chan", {"产", "缠", "馋", "掺", "蝉", "馋", "铲", "颤"}},
    {"chang", {"长", "常", "场", "唱", "厂", "尝", "昌", "畅"}},
    {"chao", {"超", "朝", "潮", "吵", "炒", "抄", "钞", "巢"}},
    {"che", {"车", "扯", "彻", "撤", "掣", "坼", "砗", "车"}},
    {"chen", {"陈", "沉", "晨", "称", "趁", "衬", "辰", "尘"}},
    {"cheng", {"成", "城", "程", "称", "诚", "承", "乘", "盛"}},
    {"chi", {"吃", "尺", "迟", "持", "池", "翅", "赤", "斥"}},
    {"chong", {"冲", "虫", "充", "重", "崇", "宠", "忡", "憧"}},
    {"chou", {"抽", "丑", "臭", "仇", "愁", "筹", "畴", "稠"}},
    {"chu", {"出", "处", "初", "除", "楚", "触", "础", "储"}},
    {"chuan", {"穿", "川", "传", "船", "串", "喘", "钏", "遄"}},
    {"chuang", {"窗", "床", "创", "闯", "疮", "幢", "怆", "窗"}},
    {"chui", {"吹", "垂", "锤", "炊", "捶", "槌", "棰", "陲"}},
    {"chun", {"春", "纯", "唇", "蠢", "醇", "淳", "鹑", "蝽"}},
    {"chuo", {"戳", "绰", "辍", "龊", "戳", "绰", "辍", "龊"}},
    {"ci", {"次", "此", "词", "刺", "磁", "瓷", "慈", "雌"}},
    {"cong", {"从", "丛", "聪", "葱", "匆", "淙", "琮", "璁"}},
    {"cou", {"凑", "辏", "腠", "凑", "辏", "腠", "凑", "辏"}},
    {"cu", {"粗", "促", "醋", "簇", "蹴", "卒", "猝", "蔟"}},
    {"cuan", {"窜", "篡", "蹿", "攒", "爨", "窜", "篡", "蹿"}},
    {"cui", {"催", "脆", "翠", "崔", "摧", "粹", "瘁", "萃"}},
    {"cun", {"村", "存", "寸", "忖", "搓", "磋", "撮", "搓"}},
    {"cuo", {"错", "措", "挫", "锉", "厝", "脞", "错", "措"}},
    {"da", {"大", "打", "达", "答", "搭", "瘩", "瘩", "达"}},
    {"dai", {"带", "代", "待", "大", "戴", "袋", "逮", "怠"}},
    {"dan", {"但", "单", "蛋", "担", "淡", "弹", "旦", "诞"}},
    {"dang", {"当", "党", "挡", "档", "荡", "铛", "裆", "当"}},
    {"dao", {"到", "道", "倒", "刀", "导", "岛", "盗", "捣"}},
    {"de", {"的", "得", "德", "地", "底", "低", "滴", "笛"}},
    {"deng", {"等", "灯", "登", "凳", "邓", "瞪", "蹬", "嶝"}},
    {"di", {"的", "地", "低", "底", "第", "弟", "递", "帝"}},
    {"dian", {"点", "电", "店", "典", "颠", "垫", "淀", "殿"}},
    {"diao", {"掉", "调", "吊", "钓", "叼", "雕", "貂", "碉"}},
    {"die", {"跌", "爹", "碟", "蝶", "叠", "谍", "牒", "垤"}},
    {"ding", {"定", "顶", "订", "丁", "钉", "盯", "叮", "酊"}},
    {"diu", {"丢", "铥", "丢", "铥", "丢", "铥", "丢", "铥"}},
    {"dong", {"动", "东", "懂", "冬", "栋", "洞", "冻", "侗"}},
    {"dou", {"都", "斗", "豆", "逗", "抖", "陡", "蚪", "窦"}},
    {"du", {"度", "读", "毒", "独", "堵", "杜", "肚", "睹"}},
    {"duan", {"段", "短", "断", "端", "锻", "缎", "椴", "煅"}},
    {"dui", {"对", "队", "堆", "兑", "怼", "憝", "队", "对"}},
    {"dun", {"顿", "吨", "蹲", "盾", "敦", "墩", "炖", "盹"}},
    {"duo", {"多", "朵", "躲", "夺", "堕", "舵", "跺", "铎"}},
    {"e", {"饿", "恶", "额", "俄", "鹅", "蛾", "讹", "鄂"}},
    {"en", {"恩", "嗯", "蒽", "恩", "嗯", "蒽", "恩", "嗯"}},
    {"er", {"二", "而", "儿", "耳", "尔", "饵", "洱", "铒"}},
    {"fa", {"发", "法", "罚", "伐", "乏", "阀", "筏", "垡"}},
    {"fan", {"反", "饭", "犯", "烦", "翻", "凡", "烦", "樊"}},
    {"fang", {"方", "放", "房", "防", "访", "仿", "纺", "芳"}},
    {"fei", {"非", "飞", "费", "肥", "废", "肺", "沸", "菲"}},
    {"fen", {"分", "份", "粉", "奋", "愤", "坟", "焚", "芬"}},
    {"feng", {"风", "封", "丰", "疯", "蜂", "锋", "峰", "逢"}},
    {"fo", {"佛", "仏", "坲", "佛", "仏", "坲", "佛", "仏"}},
    {"fou", {"否", "缶", "不", "否", "缶", "不", "否", "缶"}},
    {"fu", {"服", "父", "夫", "付", "福", "复", "副", "富"}},
    {"ga", {"嘎", "噶", "尬", "嘎", "噶", "尬", "嘎", "噶"}},
    {"gai", {"该", "改", "盖", "概", "钙", "溉", "垓", "赅"}},
    {"gan", {"干", "敢", "感", "赶", "甘", "杆", "肝", "秆"}},
    {"gang", {"刚", "钢", "港", "岗", "纲", "杠", "肛", "罡"}},
    {"gao", {"高", "搞", "告", "稿", "糕", "搞", "皋", "镐"}},
    {"ge", {"个", "歌", "各", "哥", "格", "革", "隔", "阁"}},
    {"gei", {"给", "给", "给", "给", "给", "给", "给", "给"}},
    {"gen", {"跟", "根", "亘", "艮", "茛", "跟", "根", "亘"}},
    {"geng", {"更", "耕", "庚", "羹", "埂", "耿", "梗", "哽"}},
    {"gong", {"工", "公", "共", "功", "攻", "供", "宫", "弓"}},
    {"gou", {"够", "狗", "沟", "购", "构", "钩", "勾", "垢"}},
    {"gu", {"古", "故", "顾", "骨", "鼓", "谷", "股", "孤"}},
    {"gua", {"挂", "瓜", "刮", "寡", "卦", "胍", "剐", "诖"}},
    {"guai", {"怪", "乖", "拐", "怪", "乖", "拐", "怪", "乖"}},
    {"guan", {"关", "管", "观", "官", "馆", "惯", "灌", "冠"}},
    {"guang", {"光", "广", "逛", "胱", "恍", "晃", "胱", "光"}},
    {"gui", {"贵", "归", "鬼", "规", "柜", "跪", "轨", "桂"}},
    {"gun", {"滚", "棍", "辊", "滚", "棍", "辊", "滚", "棍"}},
    {"guo", {"国", "过", "果", "锅", "裹", "卧", "涡", "蜗"}},
    {"ha", {"哈", "蛤", "铪", "哈", "蛤", "铪", "哈", "蛤"}},
    {"hai", {"还", "孩", "海", "害", "嗨", "骸", "骇", "氦"}},
    {"han", {"含", "汉", "寒", "喊", "汗", "旱", "韩", "涵"}},
    {"hang", {"行", "航", "杭", "巷", "沆", "行", "航", "杭"}},
    {"hao", {"好", "号", "浩", "耗", "豪", "毫", "郝", "嚎"}},
    {"he", {"和", "河", "喝", "何", "合", "盒", "贺", "赫"}},
    {"hei", {"黑", "嘿", "嗨", "黑", "嘿", "嗨", "黑", "嘿"}},
    {"hen", {"很", "恨", "狠", "痕", "很", "恨", "狠", "痕"}},
    {"heng", {"横", "恒", "衡", "亨", "哼", "恒", "横", "衡"}},
    {"hong", {"红", "洪", "宏", "轰", "虹", "鸿", "弘", "泓"}},
    {"hou", {"后", "厚", "候", "侯", "猴", "吼", "喉", "逅"}},
    {"hu", {"户", "湖", "胡", "虎", "护", "乎", "忽", "壶"}},
    {"hua", {"话", "花", "画", "华", "化", "划", "哗", "骅"}},
    {"huai", {"坏", "怀", "淮", "槐", "徊", "踝", "坏", "怀"}},
    {"huan", {"还", "换", "欢", "环", "缓", "幻", "焕", "唤"}},
    {"huang", {"黄", "皇", "荒", "慌", "晃", "谎", "惶", "煌"}},
    {"hui", {"会", "回", "灰", "辉", "挥", "汇", "惠", "慧"}},
    {"hun", {"混", "婚", "魂", "浑", "昏", "荤", "浑", "混"}},
    {"huo", {"或", "活", "火", "货", "获", "祸", "惑", "霍"}},
    {"ji", {"几", "机", "集", "记", "己", "及", "急", "既"}},
    {"jia", {"家", "加", "假", "价", "架", "驾", "夹", "佳"}},
    {"jian", {"见", "间", "建", "件", "简", "间", "健", "剑"}},
    {"jiang", {"将", "讲", "江", "姜", "匠", "降", "蕉", "椒"}},
    {"jiao", {"叫", "教", "交", "脚", "角", "觉", "较", "轿"}},
    {"jie", {"结", "接", "节", "街", "解", "姐", "借", "介"}},
    {"jin", {"进", "今", "金", "近", "尽", "紧", "劲", "禁"}},
    {"jing", {"经", "京", "精", "静", "境", "景", "警", "竞"}},
    {"jiong", {"窘", "迥", "炯", "窘", "迥", "炯", "窘", "迥"}},
    {"jiu", {"就", "九", "久", "酒", "旧", "救", "纠", "究"}},
    {"ju", {"句", "举", "局", "具", "剧", "聚", "拒", "据"}},
    {"juan", {"卷", "娟", "倦", "眷", "卷", "娟", "倦", "眷"}},
    {"jue", {"觉", "决", "绝", "角", "掘", "嚼", "爵", "倔"}},
    {"jun", {"军", "均", "君", "菌", "俊", "骏", "竣", "浚"}},
    {"ka", {"卡", "咖", "喀", "卡", "咖", "喀", "卡", "咖"}},
    {"kai", {"开", "凯", "慨", "楷", "铠", "锴", "开", "凯"}},
    {"kan", {"看", "砍", "刊", "勘", "堪", "瞰", "戡", "坎"}},
    {"kang", {"抗", "扛", "康", "糠", "亢", "炕", "钪", "康"}},
    {"kao", {"考", "靠", "烤", "拷", "铐", "犒", "考", "靠"}},
    {"ke", {"可", "课", "克", "客", "刻", "科", "棵", "颗"}},
    {"ken", {"肯", "啃", "垦", "恳", "裉", "肯", "啃", "垦"}},
    {"keng", {"坑", "铿", "吭", "坑", "铿", "吭", "坑", "铿"}},
    {"kong", {"空", "孔", "控", "恐", "箜", "空", "孔", "控"}},
    {"kou", {"口", "扣", "寇", "叩", "抠", "筘", "口", "扣"}},
    {"ku", {"苦", "哭", "库", "酷", "裤", "枯", "窟", "骷"}},
    {"kua", {"跨", "夸", "垮", "挎", "胯", "跨", "夸", "垮"}},
    {"kuai", {"快", "块", "筷", "会", "脍", "郐", "快", "块"}},
    {"kuan", {"宽", "款", "髋", "宽", "款", "髋", "宽", "款"}},
    {"kuang", {"况", "狂", "矿", "框", "旷", "筐", "眶", "诓"}},
    {"kui", {"亏", "愧", "溃", "葵", "魁", "馈", "聩", "睽"}},
    {"kun", {"困", "昆", "捆", "坤", "琨", "锟", "醌", "鲲"}},
    {"kuo", {"扩", "阔", "括", "廓", "蛞", "扩", "阔", "括"}},
    {"la", {"拉", "啦", "腊", "辣", "垃", "喇", "蜡", "痢"}},
    {"lai", {"来", "赖", "莱", "睐", "徕", "涞", "铼", "来"}},
    {"lan", {"兰", "蓝", "烂", "栏", "拦", "懒", "缆", "览"}},
    {"lang", {"浪", "狼", "郎", "朗", "廊", "琅", "榔", "螂"}},
    {"lao", {"老", "劳", "牢", "捞", "姥", "酪", "烙", "涝"}},
    {"le", {"了", "乐", "勒", "雷", "泪", "类", "累", "垒"}},
    {"lei", {"类", "累", "雷", "泪", "垒", "擂", "肋", "蕾"}},
    {"leng", {"冷", "愣", "棱", "冷", "愣", "棱", "冷", "愣"}},
    {"li", {"里", "力", "立", "理", "李", "例", "礼", "厉"}},
    {"lia", {"俩", "俩", "俩", "俩", "俩", "俩", "俩", "俩"}},
    {"lian", {"连", "脸", "练", "联", "恋", "莲", "廉", "帘"}},
    {"liang", {"两", "亮", "量", "良", "凉", "梁", "粮", "粱"}},
    {"liao", {"了", "料", "聊", "辽", "疗", "燎", "寥", "僚"}},
    {"lie", {"列", "烈", "猎", "裂", "劣", "冽", "洌", "趔"}},
    {"lin", {"林", "临", "邻", "淋", "琳", "霖", "鳞", "凛"}},
    {"ling", {"领", "零", "灵", "令", "另", "凌", "陵", "菱"}},
    {"liu", {"六", "留", "流", "柳", "刘", "溜", "硫", "瘤"}},
    {"long", {"龙", "隆", "笼", "聋", "拢", "垄", "珑", "胧"}},
    {"lou", {"楼", "漏", "陋", "搂", "篓", "镂", "蝼", "髅"}},
    {"lu", {"路", "录", "陆", "炉", "鲁", "鹿", "卢", "露"}},
    {"lv", {"绿", "律", "旅", "虑", "率", "铝", "屡", "履"}},
    {"luan", {"乱", "卵", "滦", "峦", "孪", "栾", "銮", "鸾"}},
    {"lun", {"论", "轮", "伦", "沦", "纶", "论", "轮", "伦"}},
    {"luo", {"落", "罗", "洛", "络", "骆", "逻", "螺", "箩"}},
    {"ma", {"吗", "妈", "马", "麻", "骂", "嘛", "码", "蚂"}},
    {"mai", {"买", "卖", "麦", "埋", "迈", "脉", "霾", "荬"}},
    {"man", {"满", "慢", "漫", "蛮", "瞒", "馒", "螨", "幔"}},
    {"mang", {"忙", "芒", "盲", "茫", "莽", "氓", "漭", "蟒"}},
    {"mao", {"毛", "猫", "冒", "帽", "茂", "貌", "贸", "茅"}},
    {"me", {"么", "嘛", "么", "嘛", "么", "嘛", "么", "嘛"}},
    {"mei", {"没", "美", "每", "妹", "眉", "梅", "媒", "煤"}},
    {"men", {"们", "门", "闷", "焖", "扪", "懑", "钔", "们"}},
    {"meng", {"梦", "猛", "蒙", "盟", "孟", "檬", "朦", "锰"}},
    {"mi", {"米", "密", "迷", "蜜", "眯", "谜", "弥", "觅"}},
    {"mian", {"面", "免", "绵", "棉", "眠", "缅", "腼", "冕"}},
    {"miao", {"秒", "苗", "庙", "妙", "描", "瞄", "渺", "缈"}},
    {"mie", {"灭", "蔑", "咩", "篾", "灭", "蔑", "咩", "篾"}},
    {"min", {"民", "敏", "闽", "悯", "皿", "抿", "泯", "珉"}},
    {"ming", {"明", "名", "命", "鸣", "铭", "冥", "暝", "瞑"}},
    {"miu", {"谬", "缪", "谬", "缪", "谬", "缪", "谬", "缪"}},
    {"mo", {"摸", "末", "墨", "默", "莫", "魔", "膜", "磨"}},
    {"mou", {"某", "谋", "眸", "牟", "某", "谋", "眸", "牟"}},
    {"mu", {"母", "木", "目", "牧", "幕", "墓", "慕", "穆"}},
    {"na", {"那", "拿", "哪", "呐", "纳", "娜", "钠", "衲"}},
    {"nai", {"奶", "耐", "乃", "奈", "萘", "鼐", "奶", "耐"}},
    {"nan", {"南", "男", "难", "楠", "喃", "腩", "蝻", "赧"}},
    {"nang", {"囊", "馕", "囔", "囊", "馕", "囔", "囊", "馕"}},
    {"nao", {"脑", "闹", "恼", "挠", "瑙", "垴", "硇", "铙"}},
    {"ne", {"呢", "讷", "呢", "讷", "呢", "讷", "呢", "讷"}},
    {"nei", {"内", "馁", "内", "馁", "内", "馁", "内", "馁"}},
    {"nen", {"嫩", "恁", "嫩", "恁", "嫩", "恁", "嫩", "恁"}},
    {"neng", {"能", "熊", "能", "熊", "能", "熊", "能", "熊"}},
    {"ni", {"你", "尼", "泥", "逆", "拟", "呢", "匿", "腻"}},
    {"nian", {"年", "念", "粘", "碾", "撵", "捻", "辇", "鲶"}},
    {"niang", {"娘", "酿", "娘", "酿", "娘", "酿", "娘", "酿"}},
    {"niao", {"鸟", "尿", "脲", "茑", "袅", "鸟", "尿", "脲"}},
    {"nie", {"捏", "涅", "聂", "孽", "镍", "涅", "蹑", "蘖"}},
    {"nin", {"您", "恁", "您", "恁", "您", "恁", "您", "恁"}},
    {"ning", {"宁", "凝", "拧", "柠", "咛", "狞", "聍", "甯"}},
    {"niu", {"牛", "扭", "妞", "钮", "纽", "忸", "牛", "扭"}},
    {"nong", {"农", "浓", "弄", "脓", "农", "浓", "弄", "脓"}},
    {"nu", {"努", "怒", "奴", "弩", "驽", "努", "怒", "奴"}},
    {"nv", {"女", "钕", "恧", "女", "钕", "恧", "女", "钕"}},
    {"nuan", {"暖", "暖", "暖", "暖", "暖", "暖", "暖", "暖"}},
    {"nue", {"虐", "疟", "虐", "疟", "虐", "疟", "虐", "疟"}},
    {"nuo", {"诺", "挪", "懦", "糯", "喏", "搦", "诺", "挪"}},
    {"o", {"哦", "噢", "喔", "哦", "噢", "喔", "哦", "噢"}},
    {"ou", {"欧", "偶", "呕", "藕", "殴", "鸥", "瓯", "怄"}},
    {"pa", {"怕", "爬", "帕", "趴", "啪", "琶", "杷", "筢"}},
    {"pai", {"排", "派", "拍", "牌", "徘", "湃", "俳", "哌"}},
    {"pan", {"盘", "判", "盼", "攀", "潘", "畔", "磐", "蹒"}},
    {"pang", {"旁", "胖", "庞", "彷", "磅", "螃", "耪", "滂"}},
    {"pao", {"跑", "炮", "泡", "抛", "袍", "刨", "咆", "庖"}},
    {"pei", {"配", "陪", "培", "佩", "赔", "沛", "佩", "辔"}},
    {"pen", {"喷", "盆", "湓", "喷", "盆", "湓", "喷", "盆"}},
    {"peng", {"朋", "碰", "彭", "捧", "蓬", "棚", "硼", "篷"}},
    {"pi", {"皮", "批", "屁", "脾", "疲", "匹", "劈", "坯"}},
    {"pian", {"片", "篇", "偏", "骗", "便", "篇", "翩", "骈"}},
    {"piao", {"票", "飘", "漂", "瓢", "嫖", "瞟", "缥", "飘"}},
    {"pie", {"撇", "瞥", "撇", "瞥", "撇", "瞥", "撇", "瞥"}},
    {"pin", {"品", "拼", "贫", "频", "聘", "嫔", "颦", "拼"}},
    {"ping", {"平", "评", "瓶", "苹", "凭", "屏", "坪", "萍"}},
    {"po", {"破", "坡", "泼", "婆", "迫", "魄", "粕", "叵"}},
    {"pou", {"剖", "掊", "裒", "剖", "掊", "裒", "剖", "掊"}},
    {"pu", {"普", "仆", "扑", "铺", "朴", "浦", "谱", "曝"}},
    {"qi", {"起", "其", "气", "期", "七", "奇", "骑", "齐"}},
    {"qia", {"恰", "洽", "掐", "袷", "髂", "恰", "洽", "掐"}},
    {"qian", {"前", "钱", "千", "浅", "签", "铅", "迁", "牵"}},
    {"qiang", {"强", "墙", "抢", "枪", "腔", "羌", "抢", "锵"}},
    {"qiao", {"桥", "敲", "巧", "瞧", "翘", "峭", "俏", "窍"}},
    {"qie", {"切", "且", "窃", "茄", "怯", "惬", "趄", "伽"}},
    {"qin", {"亲", "琴", "勤", "侵", "秦", "寝", "沁", "禽"}},
    {"qing", {"请", "清", "青", "情", "轻", "氢", "倾", "卿"}},
    {"qiong", {"穷", "琼", "穹", "茕", "蛩", "穷", "琼", "穹"}},
    {"qiu", {"球", "求", "秋", "丘", "囚", "酋", "泅", "俅"}},
    {"qu", {"去", "取", "区", "曲", "趣", "驱", "须", "虽"}},
    {"quan", {"全", "权", "圈", "泉", "拳", "犬", "券", "痊"}},
    {"que", {"却", "确", "缺", "雀", "瘸", "却", "鹊", "榷"}},
    {"qun", {"群", "裙", "逡", "群", "裙", "逡", "群", "裙"}},
    {"ran", {"然", "燃", "染", "冉", "苒", "髯", "然", "燃"}},
    {"rang", {"让", "嚷", "壤", "攘", "禳", "让", "嚷", "壤"}},
    {"rao", {"绕", "扰", "饶", "娆", "桡", "绕", "扰", "饶"}},
    {"re", {"热", "惹", "热", "惹", "热", "惹", "热", "惹"}},
    {"ren", {"人", "认", "任", "忍", "仁", "刃", "韧", "妊"}},
    {"reng", {"仍", "扔", "礽", "仍", "扔", "礽", "仍", "扔"}},
    {"ri", {"日", "驲", "日", "驲", "日", "驲", "日", "驲"}},
    {"rong", {"容", "荣", "融", "熔", "溶", "戎", "茸", "冗"}},
    {"rou", {"肉", "揉", "柔", "蹂", "鞣", "肉", "揉", "柔"}},
    {"ru", {"如", "入", "乳", "儒", "茹", "孺", "濡", "褥"}},
    {"ruan", {"软", "阮", "朊", "软", "阮", "朊", "软", "阮"}},
    {"rui", {"瑞", "锐", "蕊", "睿", "芮", "蚋", "瑞", "锐"}},
    {"run", {"润", "闰", "潤", "润", "闰", "潤", "润", "闰"}},
    {"ruo", {"若", "弱", "偌", "箬", "若", "弱", "偌", "箬"}},
    {"sa", {"撒", "洒", "萨", "卅", "飒", "撒", "洒", "萨"}},
    {"sai", {"赛", "塞", "腮", "鳃", "塞", "赛", "塞", "腮"}},
    {"san", {"三", "散", "伞", "叁", "毵", "糁", "三", "散"}},
    {"sang", {"桑", "丧", "嗓", "搡", "颡", "桑", "丧", "嗓"}},
    {"sao", {"扫", "骚", "嫂", "臊", "瘙", "扫", "骚", "嫂"}},
    {"se", {"色", "涩", "瑟", "塞", "啬", "穑", "色", "涩"}},
    {"sen", {"森", "椮", "森", "椮", "森", "椮", "森", "椮"}},
    {"seng", {"僧", "鬙", "僧", "鬙", "僧", "鬙", "僧", "鬙"}},
    {"sha", {"杀", "沙", "傻", "啥", "纱", "刹", "砂", "煞"}},
    {"shai", {"晒", "筛", "酾", "晒", "筛", "酾", "晒", "筛"}},
    {"shan", {"山", "闪", "善", "扇", "衫", "删", "杉", "珊"}},
    {"shang", {"上", "商", "伤", "赏", "裳", "晌", "熵", "垧"}},
    {"shao", {"少", "烧", "稍", "勺", "邵", "哨", "梢", "捎"}},
    {"she", {"社", "设", "射", "蛇", "舌", "舍", "涉", "摄"}},
    {"shei", {"谁", "谁", "谁", "谁", "谁", "谁", "谁", "谁"}},
    {"shen", {"什", "深", "身", "神", "甚", "肾", "慎", "渗"}},
    {"sheng", {"生", "声", "省", "圣", "胜", "盛", "剩", "牲"}},
    {"shi", {"是", "时", "事", "市", "试", "室", "式", "示"}},
    {"shou", {"手", "收", "首", "受", "瘦", "售", "守", "寿"}},
    {"shu", {"书", "数", "树", "熟", "输", "叔", "舒", "淑"}},
    {"shua", {"刷", "耍", "唰", "刷", "耍", "唰", "刷", "耍"}},
    {"shuai", {"帅", "摔", "甩", "衰", "蟀", "帅", "摔", "甩"}},
    {"shuan", {"栓", "拴", "涮", "栓", "拴", "涮", "栓", "拴"}},
    {"shuang", {"双", "爽", "霜", "孀", "骦", "鹴", "双", "爽"}},
    {"shui", {"水", "谁", "睡", "税", "吮", "水", "谁", "睡"}},
    {"shun", {"顺", "瞬", "舜", "蕣", "顺", "瞬", "舜", "蕣"}},
    {"shuo", {"说", "硕", "朔", "烁", "铄", "妁", "说", "硕"}},
    {"si", {"四", "思", "死", "私", "司", "丝", "撕", "嘶"}},
    {"song", {"送", "松", "宋", "颂", "诵", "耸", "竦", "淞"}},
    {"sou", {"搜", "艘", "嗖", "叟", "嗖", "馊", "飕", "瞍"}},
    {"su", {"苏", "速", "素", "诉", "肃", "酸", "蒜", "算"}},
    {"suan", {"算", "酸", "蒜", "狻", "算", "酸", "蒜", "狻"}},
    {"sui", {"虽", "随", "岁", "碎", "隋", "隧", "髓", "绥"}},
    {"sun", {"孙", "损", "笋", "荪", "狲", "孙", "损", "笋"}},
    {"suo", {"所", "锁", "索", "缩", "梭", "唆", "娑", "蓑"}},
    {"ta", {"他", "她", "它", "塔", "踏", "塌", "榻", "蹋"}},
    {"tai", {"太", "台", "态", "泰", "抬", "胎", "苔", "汰"}},
    {"tan", {"谈", "弹", "探", "叹", "碳", "探", "潭", "谭"}},
    {"tang", {"堂", "糖", "唐", "汤", "躺", "趟", "淌", "搪"}},
    {"tao", {"套", "逃", "桃", "淘", "涛", "掏", "滔", "韬"}},
    {"te", {"特", "忒", "慝", "特", "忒", "慝", "特", "忒"}},
    {"teng", {"疼", "腾", "藤", "滕", "誊", "疼", "腾", "藤"}},
    {"ti", {"提", "体", "题", "替", "踢", "蹄", "啼", "屉"}},
    {"tian", {"天", "田", "甜", "填", "添", "腆", "掭", "忝"}},
    {"tiao", {"条", "跳", "调", "挑", "眺", "窕", "笤", "龆"}},
    {"tie", {"铁", "贴", "帖", "餮", "铁", "贴", "帖", "餮"}},
    {"ting", {"听", "停", "厅", "廷", "挺", "庭", "艇", "亭"}},
    {"tong", {"同", "通", "痛", "统", "童", "铜", "彤", "桐"}},
    {"tou", {"头", "投", "透", "偷", "骰", "头", "投", "透"}},
    {"tu", {"图", "土", "吐", "途", "涂", "兔", "秃", "突"}},
    {"tuan", {"团", "湍", "疃", "抟", "团", "湍", "疃", "抟"}},
    {"tui", {"推", "退", "腿", "颓", "蜕", "褪", "煺", "推"}},
    {"tun", {"吞", "屯", "臀", "囤", "豚", "吞", "屯", "臀"}},
    {"tuo", {"脱", "拖", "托", "驮", "妥", "拓", "唾", "鸵"}},
    {"wa", {"挖", "哇", "蛙", "瓦", "袜", "凹", "娲", "瓦"}},
    {"wai", {"外", "歪", "崴", "外", "歪", "崴", "外", "歪"}},
    {"wan", {"完", "万", "晚", "玩", "碗", "弯", "湾", "丸"}},
    {"wang", {"王", "网", "往", "望", "忘", "旺", "妄", "枉"}},
    {"wei", {"为", "位", "围", "微", "味", "喂", "胃", "畏"}},
    {"wen", {"问", "文", "闻", "温", "稳", "吻", "瘟", "纹"}},
    {"weng", {"翁", "嗡", "瓮", "蓊", "翁", "嗡", "瓮", "蓊"}},
    {"wo", {"我", "握", "窝", "蜗", "卧", "沃", "斡", "幄"}},
    {"wu", {"五", "无", "物", "武", "午", "舞", "务", "雾"}},
    {"xi", {"西", "习", "喜", "洗", "系", "戏", "细", "吸"}},
    {"xia", {"下", "夏", "吓", "虾", "瞎", "峡", "侠", "狭"}},
    {"xian", {"先", "现", "线", "县", "鲜", "弦", "贤", "咸"}},
    {"xiang", {"想", "向", "像", "香", "响", "乡", "相", "箱"}},
    {"xiao", {"小", "笑", "校", "萧", "消", "销", "宵", "晓"}},
    {"xie", {"谢", "写", "些", "鞋", "斜", "血", "歇", "蝎"}},
    {"xin", {"新", "心", "信", "欣", "辛", "馨", "鑫", "昕"}},
    {"xing", {"行", "星", "兴", "型", "姓", "醒", "刑", "杏"}},
    {"xiong", {"雄", "熊", "凶", "兄", "胸", "匈", "汹", "雄"}},
    {"xiu", {"修", "休", "秀", "绣", "袖", "羞", "宿", "锈"}},
    {"xu", {"需", "许", "续", "须", "虚", "序", "畜", "蓄"}},
    {"xuan", {"选", "宣", "悬", "旋", "玄", "绚", "眩", "喧"}},
    {"xue", {"学", "雪", "血", "穴", "靴", "薛", "学", "雪"}},
    {"xun", {"寻", "训", "讯", "迅", "巡", "询", "循", "旬"}},
    {"ya", {"呀", "压", "牙", "鸦", "雅", "哑", "亚", "讶"}},
    {"yan", {"眼", "言", "严", "烟", "沿", "盐", "演", "艳"}},
    {"yang", {"样", "阳", "养", "央", "羊", "洋", "氧", "仰"}},
    {"yao", {"要", "药", "摇", "咬", "腰", "邀", "耀", "尧"}},
    {"ye", {"也", "夜", "叶", "业", "野", "爷", "液", "耶"}},
    {"yi", {"一", "以", "已", "意", "义", "议", "易", "医"}},
    {"yin", {"因", "音", "银", "引", "印", "饮", "隐", "阴"}},
    {"ying", {"应", "英", "影", "营", "迎", "赢", "盈", "颖"}},
    {"yo", {"哟", "唷", "哟", "唷", "哟", "唷", "哟", "唷"}},
    {"yong", {"用", "永", "勇", "拥", "涌", "庸", "佣", "臃"}},
    {"you", {"有", "又", "右", "友", "优", "游", "油", "由"}},
    {"yu", {"于", "与", "雨", "鱼", "语", "玉", "育", "欲"}},
    {"yuan", {"元", "原", "员", "圆", "园", "源", "远", "院"}},
    {"yue", {"月", "越", "约", "乐", "跃", "阅", "岳", "悦"}},
    {"yun", {"云", "运", "允", "韵", "孕", "蕴", "酝", "耘"}},
    {"za", {"杂", "咋", "砸", "匝", "咂", "杂", "咋", "砸"}},
    {"zai", {"在", "再", "载", "灾", "栽", "宰", "哉", "崽"}},
    {"zan", {"咱", "暂", "赞", "攒", "簪", "咱", "暂", "赞"}},
    {"zang", {"脏", "葬", "藏", "臧", "奘", "脏", "葬", "藏"}},
    {"zao", {"早", "造", "找", "遭", "糟", "凿", "枣", "澡"}},
    {"ze", {"则", "责", "择", "泽", "啧", "仄", "笮", "舴"}},
    {"zei", {"贼", "贼", "贼", "贼", "贼", "贼", "贼", "贼"}},
    {"zen", {"怎", "谮", "怎", "谮", "怎", "谮", "怎", "谮"}},
    {"zeng", {"增", "曾", "赠", "憎", "缯", "甑", "增", "曾"}},
    {"zha", {"扎", "炸", "渣", "眨", "榨", "咋", "札", "轧"}},
    {"zhai", {"摘", "窄", "宅", "债", "寨", "斋", "摘", "窄"}},
    {"zhan", {"站", "战", "占", "展", "战", "粘", "沾", "盏"}},
    {"zhang", {"张", "长", "章", "掌", "丈", "帐", "杖", "障"}},
    {"zhao", {"找", "照", "招", "赵", "召", "兆", "罩", "肇"}},
    {"zhe", {"这", "着", "者", "哲", "折", "涉", "浙", "蔗"}},
    {"zhei", {"这", "这", "这", "这", "这", "这", "这", "这"}},
    {"zhen", {"真", "针", "侦", "珍", "斟", "甄", "箴", "臻"}},
    {"zheng", {"正", "整", "政", "证", "征", "争", "挣", "睁"}},
    {"zhi", {"只", "知", "之", "直", "至", "治", "制", "志"}},
    {"zhong", {"中", "重", "种", "众", "钟", "终", "忠", "衷"}},
    {"zhou", {"周", "州", "洲", "舟", "粥", "轴", "肘", "帚"}},
    {"zhu", {"主", "住", "注", "猪", "竹", "助", "祝", "筑"}},
    {"zhuan", {"专", "转", "赚", "砖", "撰", "篆", "啭", "馔"}},
    {"zhuang", {"装", "状", "壮", "庄", "撞", "妆", "幢", "桩"}},
    {"zhui", {"追", "坠", "缀", "锥", "赘", "椎", "追", "坠"}},
    {"zhun", {"准", "谆", "准", "谆", "准", "谆", "准", "谆"}},
    {"zhuo", {"着", "桌", "捉", "拙", "灼", "卓", "浊", "酌"}},
    {"zi", {"子", "自", "字", "紫", "资", "姿", "滋", "孜"}},
    {"zong", {"总", "宗", "综", "踪", "棕", "鬃", "粽", "腙"}},
    {"zou", {"走", "邹", "奏", "揍", "诹", "陬", "走", "邹"}},
    {"zu", {"足", "组", "族", "阻", "祖", "租", "卒", "醉"}},
    {"zuan", {"钻", "躜", "纂", "钻", "躜", "纂", "钻", "躜"}},
    {"zui", {"最", "嘴", "醉", "罪", "最", "嘴", "醉", "罪"}},
    {"zun", {"尊", "遵", "樽", "鳟", "尊", "遵", "樽", "鳟"}},
    {"zuo", {"做", "作", "坐", "左", "座", "昨", "佐", "做作"}},
};

#define CANDIDATE_TABLE_SIZE (sizeof(candidateTable) / sizeof(candidateTable[0]))
const char* currentCandidates[8];
int currentCandidateCount = 0;

static void wsEventCallback(WStype_t type, uint8_t* payload, size_t length) {
    chat.handleWsEvent(type, payload, length);
}

void ChatManager::init() {
    _active = false;
    _view = CHAT_VIEW_LIST;
    _selectedConv = 0;
    _convScrollOffset = 0;
    _messageScrollOffset = 0;
    _convCount = 0;
    _messageCount = 0;
    _inputMode = INPUT_NONE;
    _gridCursor = 4;
    _letterCursor = 0;
    _pinyinLen = 0;
    _pinyin[0] = 0;
    _candidateIndex = 0;
    _inputBufferLen = 0;
    _inputBuffer[0] = 0;
    _wsConnected = false;
    _loggedIn = false;
    _userId[0] = 0;
    _userName[0] = 0;
    _token[0] = 0;
    _lastReconnectTime = 0;
    addConversation("public", "网站问题反馈区", CONV_PUBLIC);
}

void ChatManager::enter() {
    _active = true;
    _view = CHAT_VIEW_LIST;
    _selectedConv = 0;
    wifiConfig.ensureConnected();  // 自动尝试连接 WiFi
    if (!_wsConnected) {
        drawConnecting();
        connectWebSocket();
    } else {
        drawConversationList();
    }
}

void ChatManager::exit() {
    _active = false;
    _inputMode = INPUT_NONE;
}

void ChatManager::addConversation(const char* id, const char* name, ConvType type) {
    if (_convCount >= MAX_CONVERSATIONS) return;
    for (int i = 0; i < _convCount; i++) {
        if (strcmp(_conversations[i].id, id) == 0) {
            strncpy(_conversations[i].name, name, 31);
            _conversations[i].name[31] = 0;
            return;
        }
    }
    strncpy(_conversations[_convCount].id, id, 31);
    _conversations[_convCount].id[31] = 0;
    strncpy(_conversations[_convCount].name, name, 31);
    _conversations[_convCount].name[31] = 0;
    _conversations[_convCount].type = type;
    _conversations[_convCount].unread = 0;
    _conversations[_convCount].valid = true;
    _convCount++;
}

Conversation* ChatManager::getCurrentConversation() {
    if (_selectedConv >= 0 && _selectedConv < _convCount) {
        return &_conversations[_selectedConv];
    }
    return nullptr;
}

void ChatManager::addMessage(const char* from, const char* fromName, const char* content, bool isMe) {
    if (_messageCount >= MAX_MESSAGES_PER_CONV) {
        for (int i = 1; i < MAX_MESSAGES_PER_CONV; i++) {
            _messages[i-1] = _messages[i];
        }
        _messageCount--;
    }
    strncpy(_messages[_messageCount].from, from, 31);
    _messages[_messageCount].from[31] = 0;
    strncpy(_messages[_messageCount].fromName, fromName, 31);
    _messages[_messageCount].fromName[31] = 0;
    strncpy(_messages[_messageCount].content, content, 63);
    _messages[_messageCount].content[63] = 0;
    _messages[_messageCount].isMe = isMe;
    _messages[_messageCount].time = millis();
    _messageCount++;
}

void ChatManager::clearCurrentMessages() {
    _messageCount = 0;
    _messageScrollOffset = 0;
}

void ChatManager::clearAllCache() {
    clearCurrentMessages();
    for (int i = 0; i < _convCount; i++) {
        _conversations[i].unread = 0;
    }
}

void ChatManager::drawConnecting() {
    disp.clear();
    disp.drawTitleBar("虚空终端");
    disp.drawText(SCREEN_W/2 - 30, SCREEN_H/2 - 10, "连接中...", 2);
    disp.drawProgressBar(20, SCREEN_H/2 + 15, SCREEN_W - 40, 30);
    disp.drawStatusBar("正在连接服务器", "Home:返回");
    disp.refresh(true);
}

void ChatManager::drawConversationList() {
    disp.clear();
    disp.drawTitleBar("虚空终端 - 消息");
    if (_convCount == 0) {
        disp.drawText(SCREEN_W/2 - 30, SCREEN_H/2, "暂无会话", 2);
    } else {
        int y = 20;
        int visibleCount = 0;
        int maxVisible = 7;
        for (int i = _convScrollOffset; i < _convCount && visibleCount < maxVisible; i++) {
            bool selected = (i == _selectedConv);
            if (selected) {
                disp.drawRect(0, y - 2, SCREEN_W, 16, true);
                u8g2Fonts.setForegroundColor(GxEPD_WHITE);
                u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
            }
            const char* typeIcon = "";
            switch (_conversations[i].type) {
                case CONV_PUBLIC: typeIcon = "[公]"; break;
                case CONV_GROUP: typeIcon = "[群]"; break;
                case CONV_PRIVATE: typeIcon = "[私]"; break;
            }
            char line[80];
            if (_conversations[i].unread > 0) {
                snprintf(line, sizeof(line), "%s %s (%d)", typeIcon, _conversations[i].name, _conversations[i].unread);
            } else {
                snprintf(line, sizeof(line), "%s %s", typeIcon, _conversations[i].name);
            }
            disp.drawText(4, y, line, 1);
                u8g2Fonts.setForegroundColor(GxEPD_BLACK);
                u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
            y += 14;
            visibleCount++;
        }
    }
    char statusLine[64];
    if (_wsConnected) {
        snprintf(statusLine, sizeof(statusLine), "已连接 | %s", _loggedIn ? _userName : "未登录");
    } else {
        snprintf(statusLine, sizeof(statusLine), "未连接");
    }
    disp.drawStatusBar(statusLine, "上/下:选择 长按下:进入 Home:返回");
    disp.refresh(true);
}

void ChatManager::drawChatView() {
    disp.clear();
    Conversation* conv = getCurrentConversation();
    char title[64];
    if (conv) {
        const char* typeStr = "";
        switch (conv->type) {
            case CONV_PUBLIC: typeStr = "公共大厅"; break;
            case CONV_GROUP: typeStr = "群聊"; break;
            case CONV_PRIVATE: typeStr = "私聊"; break;
        }
        snprintf(title, sizeof(title), "%s - %s", typeStr, conv->name);
    } else {
        snprintf(title, sizeof(title), "聊天");
    }
    disp.drawTitleBar(title);
    int y = SCREEN_H - 14;
    int visibleCount = 0;
    int maxVisible = 6;
    for (int i = _messageCount - 1 - _messageScrollOffset; i >= 0 && visibleCount < maxVisible; i--) {
        ChatMessage& msg = _messages[i];
        char senderLine[64];
        snprintf(senderLine, sizeof(senderLine), "%s:", msg.fromName);
        y -= 16;
        if (y < 18) break;
        if (msg.isMe) {
            int w = disp.getTextWidth(senderLine);
            disp.drawText(SCREEN_W - w - 4, y, senderLine, 1);
        } else {
            disp.drawText(4, y, senderLine, 1);
        }
        // 消息内容 UTF-8 安全分行
        int lineLens[8];
        int lineCount = 0;
        int cpos = 0;
        while (msg.content[cpos] && lineCount < 8) {
            int lb = disp.measureLine(msg.content + cpos, SCREEN_W - 16);
            if (lb <= 0) break;
            lineLens[lineCount++] = lb;
            cpos += lb;
        }
        cpos = 0;
        for (int k = 0; k < lineCount; k++) {
            y -= 16;
            if (y < 18) break;
            int n = lineLens[k];
            if (n > 63) n = 63;
            char line[64];
            memcpy(line, msg.content + cpos, n);
            if (n > 0 && line[n-1] == '\n') n--;
            line[n] = 0;
            cpos += lineLens[k];
            if (msg.isMe) {
                int w = disp.getTextWidth(line);
                disp.drawText(SCREEN_W - w - 8, y, line, 1);
            } else {
                disp.drawText(8, y, line, 1);
            }
        }
        visibleCount++;
        y -= 2;
    }
    if (_inputMode != INPUT_NONE) {
        disp.drawRect(0, SCREEN_H - 14, SCREEN_W, 14, true);
        display.setTextColor(COLOR_WHITE);
        char inputLine[100];
        snprintf(inputLine, sizeof(inputLine), "%s%s", _inputBuffer, _pinyin);
        disp.drawText(2, SCREEN_H - 12, inputLine, 1);
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    } else {
        disp.drawStatusBar("上/下:翻页 双击下:输入 短按上:返回列表", "Home:主页");
    }
    disp.refresh(true);
}

void ChatManager::drawInputKeyboard() {
    disp.clear();
    disp.drawTitleBar("输入消息");
    
    // 左右分屏：左边输入框(110px)，右边九宫格(186px)
    int leftW = 110;
    int rightX = leftW;
    int rightW = SCREEN_W - leftW;
    int contentY = 16;
    int contentH = SCREEN_H - 16 - 14; // 减去标题栏和状态栏
    
    // ===== 左边：输入框区域 =====
    // 候选词栏
    int candY = contentY;
    int candH = 32;
    disp.drawRect(0, candY, leftW, candH, false);
    if (_pinyinLen > 0 && currentCandidateCount > 0) {
        char candLine[80];
        int pos = 0;
        for (int i = 0; i < currentCandidateCount && pos < 70; i++) {
            if (i == _candidateIndex) candLine[pos++] = '[';
            const char* cand = currentCandidates[i];
            while (*cand && pos < 65) candLine[pos++] = *cand++;
            if (i == _candidateIndex) candLine[pos++] = ']';
            candLine[pos++] = ' ';
        }
        candLine[pos] = 0;
        disp.drawText(2, candY + 2, _pinyin, 1);
        disp.drawText(2, candY + 14, candLine, 1);
    } else {
        disp.drawText(2, candY + 10, "选字母组词", 1);
    }
    
    // 已输入文字区域
    int inputY = candY + candH + 2;
    int inputH = contentH - candH - 2 - 30; // 减去发送按钮高度
    disp.drawRect(0, inputY, leftW, inputH, false);
    if (_inputBufferLen > 0) {
        // 按 UTF-8 边界自动换行显示已输入文字
        int y = inputY + 2;
        int cpos = 0;
        while (cpos < _inputBufferLen && y < inputY + inputH - 10) {
            int lb = disp.measureLine(_inputBuffer + cpos, leftW - 4);
            if (lb <= 0) { cpos++; continue; }
            int n = lb;
            if (n > 31) n = 31;
            char line[32];
            memcpy(line, _inputBuffer + cpos, n);
            if (n > 0 && line[n-1] == '\n') n--;
            line[n] = 0;
            disp.drawText(2, y, line, 1);
            y += 16;
            cpos += lb;
        }
    } else {
        disp.drawText(2, inputY + 10, "已输入:", 1);
    }
    
    // 发送按钮
    int sendY = inputY + inputH + 2;
    int sendH = 26;
    bool canSend = (_inputBufferLen > 0);
    if (canSend) {
        disp.drawRect(2, sendY, leftW - 4, sendH, true);
        display.setTextColor(COLOR_WHITE);
        disp.drawText(leftW/2 - 12, sendY + 8, "发送", 2);
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    } else {
        disp.drawRect(2, sendY, leftW - 4, sendH, false);
        disp.drawText(leftW/2 - 12, sendY + 8, "发送", 2);
    }
    
    // ===== 右边：十二宫格键盘（3x4：9字母 + 删除/空格/发送）=====
    int keyW = rightW / 3;
    int keyH = contentH / 4;
    int startY = contentY;
    
    for (int i = 0; i < 12; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = rightX + col * keyW;
        int y = startY + row * keyH;
        
        bool selected = (i == _gridCursor);
        if (selected) {
            disp.drawRect(x + 2, y + 2, keyW - 4, keyH - 4, true);
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
            u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
        } else {
            disp.drawRect(x + 2, y + 2, keyW - 4, keyH - 4, false);
        }
        
        if (i < 9) {
            // 字母格
            char numStr[2] = {(char)('1' + i), 0};
            disp.drawText(x + 4, y + 3, numStr, 1);
            disp.drawText(x + 4, y + 15, getGridLetters(i), 1);
        } else if (i == 9) {
            // 删除键
            disp.drawText(x + keyW/2 - 7, y + keyH/2 - 5, "删", 1);
        } else if (i == 10) {
            // 空格键
            disp.drawText(x + keyW/2 - 14, y + keyH/2 - 5, "空格", 1);
        } else {
            // 发送键
            disp.drawText(x + keyW/2 - 14, y + keyH/2 - 5, "发送", 1);
        }
        
        if (selected) {
            u8g2Fonts.setForegroundColor(GxEPD_BLACK);
            u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
        }
    }
    
    // 底部状态栏
    disp.drawStatusBar("2/3:移动 长按3:确认 长按2:切换候选", nullptr);
    disp.refresh(true);
}

void ChatManager::drawLetterSelector() {
    disp.clear();
    disp.drawTitleBar("选择字母");
    const char* letters = getGridLetters(_gridCursor);
    int count = strlen(letters);
    int boxW = SCREEN_W / count;
    for (int i = 0; i < count; i++) {
        int x = i * boxW;
        bool selected = (i == _letterCursor);
        if (selected) {
            disp.drawRect(x + 4, 40, boxW - 8, 40, true);
            display.setTextColor(COLOR_WHITE);
        } else {
            disp.drawRect(x + 4, 40, boxW - 8, 40, false);
        }
        char letter[2] = {letters[i], 0};
        disp.drawText(x + boxW/2 - 4, 52, letter, 2);
        if (selected) display.setTextColor(COLOR_BLACK);
    }
    disp.drawStatusBar("3:下一个 双击3:上一个 长按3:确认 2:返回", nullptr);
    disp.refresh(true);
}

void ChatManager::connectWebSocket() {
    if (WiFi.status() != WL_CONNECTED) return;
    // 自动登录：未登录且无 token 时，用配网配置的账号登录
    if (!_loggedIn && !_token[0]) {
        char u[CHAT_USERNAME_MAX];
        char p[CHAT_PASSWORD_MAX];
        wifiConfig.loadChatAccount(u, CHAT_USERNAME_MAX, p, CHAT_PASSWORD_MAX);
        if (u[0] && p[0]) {
            login(u, p);
        }
    }
    _webSocket.beginSSL(CHAT_SERVER, CHAT_PORT, "/ws", chatSslFingerprint);
    _webSocket.onEvent(wsEventCallback);
    _webSocket.setReconnectInterval(5000);
    _wsConnected = true;
}

void ChatManager::disconnectWebSocket() {
    _webSocket.disconnect();
    _wsConnected = false;
    _loggedIn = false;
}

void ChatManager::sendWsMessage(const char* type, const char* to, const char* content) {
    if (!_wsConnected) return;
    StaticJsonDocument<512> doc;
    doc["type"] = type;
    if (to) doc["to"] = to;
    if (content) doc["content"] = content;
    if (_token[0]) doc["token"] = _token;
    // auth 消息带 lite 标志，服务器返回精简 hello（跳过历史消息大头）
    if (strcmp(type, "auth") == 0) doc["lite"] = true;
    String json;
    serializeJson(doc, json);
    _webSocket.sendTXT(json);
}

void ChatManager::sendChatMessage(Conversation* conv, const char* content) {
    if (!_wsConnected || !conv || !content) return;
    StaticJsonDocument<512> doc;
    if (conv->type == CONV_PUBLIC) {
        doc["type"] = "global";
    } else if (conv->type == CONV_GROUP) {
        doc["type"] = "group";
        doc["gid"] = conv->id;
    } else {
        doc["type"] = "dm";
        doc["to"] = conv->id;
    }
    doc["content"] = content;
    String json;
    serializeJson(doc, json);
    _webSocket.sendTXT(json);
}

void ChatManager::handleWsEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            _wsConnected = false;
            _loggedIn = false;
            break;
        case WStype_CONNECTED:
            _wsConnected = true;
            if (_token[0]) sendWsMessage("auth", nullptr, nullptr);
            break;
        case WStype_TEXT: {
            StaticJsonDocument<4096> doc;
            if (deserializeJson(doc, payload, length)) break;
            const char* msgType = doc["type"] | "";
            if (strcmp(msgType, "hello") == 0) {
                handleHelloMessage(doc.as<JsonObject>());
            } else if (strcmp(msgType, "global") == 0) {
                handleGlobalMessage(doc.as<JsonObject>());
            } else if (strcmp(msgType, "dm") == 0) {
                handleDmMessage(doc.as<JsonObject>());
            } else if (strcmp(msgType, "group") == 0) {
                handleGroupMessage(doc.as<JsonObject>());
            }
            break;
        }
        default: break;
    }
}

void ChatManager::handleHelloMessage(JsonObject root) {
    // 解析自己的信息（auth 成功后服务器通过 hello.self 下发）
    JsonObject self = root["self"].as<JsonObject>();
    if (!self.isNull()) {
        const char* uid = self["id"] | "";
        const char* uname = self["username"] | "";
        if (uid[0]) {
            strncpy(_userId, uid, 31);
            _userId[31] = 0;
            _loggedIn = true;
        }
        if (uname[0]) {
            strncpy(_userName, uname, 31);
            _userName[31] = 0;
        }
    }
    JsonArray friends = root["friends"].as<JsonArray>();
    for (JsonObject friendObj : friends) {
        const char* fid = friendObj["id"] | "";
        const char* fname = friendObj["name"] | "";
        if (fid[0]) addConversation(fid, fname, CONV_PRIVATE);
    }
    JsonArray groups = root["groups"].as<JsonArray>();
    for (JsonObject groupObj : groups) {
        const char* gid = groupObj["id"] | "";
        const char* gname = groupObj["name"] | "";
        if (gid[0]) addConversation(gid, gname, CONV_GROUP);
    }
    JsonArray globalMsgs = root["globalMsgs"].as<JsonArray>();
    for (JsonObject msgObj : globalMsgs) {
        const char* from = msgObj["from"] | "";
        const char* fromName = msgObj["fromName"] | from;
        const char* content = msgObj["content"] | "";
        bool isMe = (strcmp(from, _userId) == 0);
        if (_selectedConv == 0 && strcmp(_conversations[0].id, "public") == 0) {
            addMessage(from, fromName, content, isMe);
        }
    }
    if (_active && _view == CHAT_VIEW_LIST) drawConversationList();
}

void ChatManager::handleGlobalMessage(JsonObject root) {
    const char* from = root["from"] | "";
    const char* fromName = root["fromName"] | from;
    const char* content = root["content"] | "";
    bool isMe = (strcmp(from, _userId) == 0);
    Conversation* conv = getCurrentConversation();
    if (conv && conv->type == CONV_PUBLIC) {
        if (_view == CHAT_VIEW_CHAT) {
            addMessage(from, fromName, content, isMe);
            drawChatView();
        } else if (_convCount > 0) {
            _conversations[0].unread++;
            drawConversationList();
        }
    }
}

void ChatManager::handleDmMessage(JsonObject root) {
    const char* from = root["from"] | "";
    const char* fromName = root["fromName"] | from;
    const char* content = root["content"] | "";
    const char* to = root["to"] | "";
    bool isMe = (strcmp(from, _userId) == 0);
    Conversation* conv = getCurrentConversation();
    if (conv && conv->type == CONV_PRIVATE) {
        // 私聊：对方是 from 或 to 中不等于自己的那一个
        const char* peer = isMe ? to : from;
        if (strcmp(conv->id, peer) == 0 && _view == CHAT_VIEW_CHAT) {
            addMessage(from, fromName, content, isMe);
            drawChatView();
        }
    }
}

void ChatManager::handleGroupMessage(JsonObject root) {
    const char* from = root["from"] | "";
    const char* fromName = root["fromName"] | from;
    const char* content = root["content"] | "";
    const char* gid = root["gid"] | "";
    bool isMe = (strcmp(from, _userId) == 0);
    Conversation* conv = getCurrentConversation();
    if (conv && conv->type == CONV_GROUP && strcmp(conv->id, gid) == 0) {
        if (_view == CHAT_VIEW_CHAT) {
            addMessage(from, fromName, content, isMe);
            drawChatView();
        }
    }
}

bool ChatManager::login(const char* username, const char* password) {
    if (WiFi.status() != WL_CONNECTED) return false;
    WiFiClientSecure client;
    client.setFingerprint(chatSslFingerprint);
    client.setTimeout(5000);
    HTTPClient http;
    http.setTimeout(5000);
    String url = String("https://") + CHAT_SERVER + "/api/login";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    StaticJsonDocument<256> doc;
    doc["username"] = username;
    doc["password"] = password;
    String body;
    serializeJson(doc, body);
    int httpCode = http.POST(body);
    if (httpCode == 200) {
        String payload = http.getString();
        StaticJsonDocument<512> resp;
        if (!deserializeJson(resp, payload)) {
            const char* token = resp["token"] | "";
            if (token[0]) {
                strncpy(_token, token, 63);
                _loggedIn = true;
                http.end();
                return true;
            }
        }
    }
    http.end();
    return false;
}

const char* ChatManager::getGridLetters(int index) {
    static char buf[8];
    if (index >= 0 && index < 9) {
        const char* str = (const char*)pgm_read_ptr(&gridLetters[index]);
        strcpy(buf, str);
        return buf;
    }
    return "";
}

void ChatManager::appendPinyin(char c) {
    if (_pinyinLen < 30) {
        _pinyin[_pinyinLen++] = c;
        _pinyin[_pinyinLen] = 0;
        loadCandidates();
    }
}

void ChatManager::deletePinyin() {
    if (_pinyinLen > 0) {
        _pinyinLen--;
        _pinyin[_pinyinLen] = 0;
        loadCandidates();
    }
}

void ChatManager::loadCandidates() {
    currentCandidateCount = 0;
    _candidateIndex = 0;
    
    if (_pinyinLen == 0) return;
    
    static char candidateBuffer[8][32];
    char pinyinBuf[16];
    char candBuf[32];
    
    // 1. 先尝试全拼精确匹配
    for (unsigned int i = 0; i < CANDIDATE_TABLE_SIZE && currentCandidateCount < 8; i++) {
        const char* pinyin = (const char*)pgm_read_ptr(&candidateTable[i].pinyin);
        strcpy(pinyinBuf, pinyin);
        if (strcmp(pinyinBuf, _pinyin) == 0) {
            for (int j = 0; j < 8 && currentCandidateCount < 8; j++) {
                const char* cand = (const char*)pgm_read_ptr(&candidateTable[i].candidates[j]);
                if (!cand) break;
                strcpy(candBuf, cand);
                strcpy(candidateBuffer[currentCandidateCount], candBuf);
                currentCandidates[currentCandidateCount] = candidateBuffer[currentCandidateCount];
                currentCandidateCount++;
            }
            break;
        }
    }
    
    // 2. 如果全拼没匹配到，或者输入只有1个字母（首字母模式），尝试首字母匹配
    if (currentCandidateCount == 0 || _pinyinLen == 1) {
        for (unsigned int i = 0; i < CANDIDATE_TABLE_SIZE && currentCandidateCount < 8; i++) {
            const char* pinyin = (const char*)pgm_read_ptr(&candidateTable[i].pinyin);
            strcpy(pinyinBuf, pinyin);
            // 检查拼音是否以输入的首字母开头
            if (strncmp(pinyinBuf, _pinyin, _pinyinLen) == 0) {
                for (int j = 0; j < 8 && currentCandidateCount < 8; j++) {
                    const char* cand = (const char*)pgm_read_ptr(&candidateTable[i].candidates[j]);
                    if (!cand) break;
                    strcpy(candBuf, cand);
                    // 避免重复
                    bool exists = false;
                    for (int k = 0; k < currentCandidateCount; k++) {
                        if (strcmp(currentCandidates[k], candBuf) == 0) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        strcpy(candidateBuffer[currentCandidateCount], candBuf);
                        currentCandidates[currentCandidateCount] = candidateBuffer[currentCandidateCount];
                        currentCandidateCount++;
                    }
                }
            }
        }
    }
    
    // 3. 如果还是没有，显示拼音本身
    if (currentCandidateCount == 0) {
        strcpy(candidateBuffer[0], _pinyin);
        currentCandidates[0] = candidateBuffer[0];
        currentCandidateCount = 1;
    }
}

void ChatManager::confirmCandidate() {
    if (_candidateIndex < currentCandidateCount) {
        const char* cand = currentCandidates[_candidateIndex];
        int candLen = strlen(cand);
        if (_inputBufferLen + candLen < 63) {
            strcpy(_inputBuffer + _inputBufferLen, cand);
            _inputBufferLen += candLen;
        }
    }
    // 上屏后清空拼音，回到九宫格继续输入（不立即发送，由长按按键2发送）
    _pinyinLen = 0;
    _pinyin[0] = 0;
    currentCandidateCount = 0;
    _candidateIndex = 0;
    drawInputKeyboard();
}

void ChatManager::deleteInputChar() {
    if (_pinyinLen > 0) {
        // 优先删除拼音
        deletePinyin();
        return;
    }
    if (_inputBufferLen <= 0) return;
    // UTF-8 安全删除：中文字符占 3 字节，需整体删除
    if ((unsigned char)_inputBuffer[_inputBufferLen - 1] >= 0x80) {
        int n = 0;
        while (_inputBufferLen > 0 && (unsigned char)_inputBuffer[_inputBufferLen - 1] >= 0x80 && n < 3) {
            _inputBufferLen--;
            n++;
        }
    } else {
        _inputBufferLen--;
    }
    _inputBuffer[_inputBufferLen] = 0;
}

void ChatManager::handleKey(KeyEvent evt) {
    if (!_active || evt == KEY_NONE) return;
    if (evt == KEY_MENU_SHORT) {
        // 按键1短按 = 返回首页（任何模式下都返回）
        exit();
        app.goHome();
        return;
    }
    if (_view == CHAT_VIEW_LIST) {
        if (evt == KEY_UP_SHORT) {
            if (_selectedConv > 0) {
                _selectedConv--;
                if (_selectedConv < _convScrollOffset) _convScrollOffset = _selectedConv;
                drawConversationList();
            }
        } else if (evt == KEY_DOWN_SHORT) {
            if (_selectedConv < _convCount - 1) {
                _selectedConv++;
                if (_selectedConv >= _convScrollOffset + 7) _convScrollOffset = _selectedConv - 6;
                drawConversationList();
            }
        } else if (evt == KEY_DOWN_LONG) {
            clearCurrentMessages();
            _view = CHAT_VIEW_CHAT;
            _messageScrollOffset = 0;
            drawChatView();
        } else if (evt == KEY_UP_LONG) {
            clearAllCache();
            drawConversationList();
        }
    } else if (_view == CHAT_VIEW_CHAT) {
        if (_inputMode == INPUT_NONE) {
            if (evt == KEY_UP_SHORT) {
                if (_messageScrollOffset < _messageCount - 1) {
                    _messageScrollOffset++;
                    drawChatView();
                }
            } else if (evt == KEY_DOWN_SHORT) {
                if (_messageScrollOffset > 0) {
                    _messageScrollOffset--;
                    drawChatView();
                }
            } else if (evt == KEY_DOWN_DOUBLE) {
                _inputMode = INPUT_GRID;
                _gridCursor = 4;
                _pinyinLen = 0;
                _pinyin[0] = 0;
                _candidateIndex = 0;
                currentCandidateCount = 0;
                drawInputKeyboard();
            } else if (evt == KEY_UP_LONG) {
                _view = CHAT_VIEW_LIST;
                _inputMode = INPUT_NONE;
                drawConversationList();
            }
        } else if (_inputMode == INPUT_GRID) {
            // 十二宫格光标移动：单击上下，双击左右，长按确认
            if (evt == KEY_UP_SHORT) {
                // 按键2单击：上移一行
                if (_gridCursor >= 3) _gridCursor -= 3;
                drawInputKeyboard();
            } else if (evt == KEY_DOWN_SHORT) {
                // 按键3单击：下移一行
                if (_gridCursor < 9) _gridCursor += 3;
                drawInputKeyboard();
            } else if (evt == KEY_UP_DOUBLE) {
                // 按键2双击：左移一列（同行内循环）
                int col = _gridCursor % 3;
                if (col > 0) _gridCursor--;
                else _gridCursor += 2;
                drawInputKeyboard();
            } else if (evt == KEY_DOWN_DOUBLE) {
                // 按键3双击：右移一列（同行内循环）
                int col = _gridCursor % 3;
                if (col < 2) _gridCursor++;
                else _gridCursor -= 2;
                drawInputKeyboard();
            } else if (evt == KEY_DOWN_LONG) {
                // 按键3长按：确认当前格
                if (_gridCursor < 9) {
                    // 字母格：有候选词则上屏，否则进入字母选择
                    if (currentCandidateCount > 0 && _pinyinLen > 0) {
                        confirmCandidate();
                    } else {
                        _inputMode = INPUT_LETTER;
                        _letterCursor = 0;
                        drawLetterSelector();
                    }
                } else if (_gridCursor == 9) {
                    // 删除键
                    deleteInputChar();
                    drawInputKeyboard();
                } else if (_gridCursor == 10) {
                    // 空格键
                    if (_inputBufferLen < 63) {
                        _inputBuffer[_inputBufferLen++] = ' ';
                        _inputBuffer[_inputBufferLen] = 0;
                    }
                    drawInputKeyboard();
                } else if (_gridCursor == 11) {
                    // 发送键
                    if (_inputBufferLen > 0) {
                        Conversation* conv = getCurrentConversation();
                        if (conv) {
                            sendChatMessage(conv, _inputBuffer);
                            addMessage(_userId, _userName, _inputBuffer, true);
                        }
                        _inputBufferLen = 0;
                        _inputBuffer[0] = 0;
                        _pinyinLen = 0;
                        _pinyin[0] = 0;
                        currentCandidateCount = 0;
                        _candidateIndex = 0;
                        _inputMode = INPUT_NONE;
                        drawChatView();
                    }
                }
            } else if (evt == KEY_UP_LONG) {
                // 按键2长按：有候选词切换候选词，无候选词退出输入模式
                if (currentCandidateCount > 1) {
                    _candidateIndex = (_candidateIndex + 1) % currentCandidateCount;
                    drawInputKeyboard();
                } else {
                    _inputMode = INPUT_NONE;
                    _pinyinLen = 0;
                    _pinyin[0] = 0;
                    currentCandidateCount = 0;
                    drawChatView();
                }
            }
        } else if (_inputMode == INPUT_LETTER) {
            // 字母选择：按键3单击下一个、双击上一个，长按确认；按键2短按返回
            const char* letters = getGridLetters(_gridCursor);
            int count = strlen(letters);
            if (evt == KEY_UP_SHORT) {
                _inputMode = INPUT_GRID;
                drawInputKeyboard();
            } else if (evt == KEY_DOWN_SHORT) {
                _letterCursor = (_letterCursor + 1) % count;
                drawLetterSelector();
            } else if (evt == KEY_DOWN_DOUBLE) {
                _letterCursor = (_letterCursor + count - 1) % count;
                drawLetterSelector();
            } else if (evt == KEY_DOWN_LONG) {
                appendPinyin(letters[_letterCursor]);
                _inputMode = INPUT_GRID;
                drawInputKeyboard();
            }
        }
    }
}

void ChatManager::update() {
    if (!_active) return;
    // WiFi 未连接时持续尝试（后台连接）
    if (WiFi.status() != WL_CONNECTED) {
        wifiConfig.ensureConnected();
    }
    if (_wsConnected) {
        _webSocket.loop();
    } else if (millis() - _lastReconnectTime > 5000) {
        _lastReconnectTime = millis();
        if (WiFi.status() == WL_CONNECTED) {
            connectWebSocket();
        }
    }
}
