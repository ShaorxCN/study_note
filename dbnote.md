基于mysql  部分最常见的 其他高级特性等见onenote 

- [DQL](#c1)
- [DML](#c2)
- [DDL](#c3)
- [DCL](#c4)


<div id=c1><h2>DQL   data query language</h2></div>

子句位置必须符合   from  xxx    where xxx  group by xxx  having xxxx order by   xxxx  limit  xxxx offset xxxx



Select  Distinct columnName from xxx;去重查询


 select * from spider_user limit 5,6//开始位置为5（位置从0开始，也就是说第六条记录开始），查询6条记录


select * from spider_user limit 6 offset 5效果同上,offset后接偏移值。也就是从哪儿开始

select * from xxxx order by column [desc][,column [desc]].默认正序，desc为逆序。多字段排序是在前者的基础上再排


多个查询过滤子句AND OR 其中and优先级大于OR
        
select * from spider_user where sex = '2'  or sex = '1' and vacation = '互联网'

//这上面是（sex = '2'）or (sex='1' and vacation ='互联网')也即是说会出现sex为1的情况，但是肯定是vacation值是互联网


跨库查询+完全限定名
select a.b from c.a;c为库名，a为表名，b为字段名  当然需要用户具有库的查询权限,不然会出错显示命令拒绝


拼接字段 Concat()函数实现将对象拼接起来

select concat(username,'( ',name,' )') from spider_user

结果即使username( name )这种格式的数据返回.同时可以通过使用别名来显示field


聚合函数
Avg() 
Min() 
Max()
Sum()
Count() tip之一count(column)智力就会只统计column列有值的行数


分组查询

Group by这时候的聚集函数就是针对分组数据生效的
除了计算聚合函数以外，select后的每一个都必须出现在groupz by 中

select vacation,count(*) as nums from spider_user where sex in('0','1') group by vacation having nums>=2 order by vacation

等值联结 
Select a.x,b.x from a,b where a.xxx = b.xxx

内联结
Select a.x,b.x from a inner join b on a.xxx = b.xxx

自联结
比如查询一个表t，a,b两个字段.a为主键，b会重复.给出a= id找出表中于id同b属性的id的值
Select t1.a,t2.b from t as t1,t as t2 where t1.a = t2.b and t2.a =id
select * from spider_user as a,spider_user as b where a.vacation = b.vacation and b.username = 'afunnyboy'


外部联结
Left join
Right join
select * from spider_user a left join spider_user b on a.username = b.username and b.sex = '1'

什么联结就保证那边的表的数据是全的。另外一边如果没有数据就是null

select a.vacation,count(*) from spider_user a left join spider_user b on a.username = b.username and b.sex = '1' group ba.vacation


<div id=c2><h2>DML   data Manipulation language</h2></div>

Insert

insert into targetName(columnName[,columnName]…) values(…)[,(…)]
insert into enumtest values(123,'123','fmale'),(234,'234','male') // 默认全col
    
Update and delete
    
Update targetName set columnName = xxx [,columnName2 = xxxx] where xxx = xxxx
Delete from targetName where xxx=xxx


<div id=c3><h2>DDL   data definition  language</h2></div>

create table customers(
    cust_id int not null  auto_increment,
    cust_name varchar(50) not null,
    cust_address varchar(100) null,
    cust_city varchar(50) null,
    cust_state varchar(5) null,
    cust_zip varchar(8) null,
    cust_country varchar(50) null,
    cust_contact varchar(100) null,
    cust_email varchar(255) null,
    primary key(cust_id)
)engine = InnoDb default charset=utf8;


create table customers(
    cust_id int not null  auto_increment,
    cust_name varchar(50) not null,
    cust_address varchar(100) null,
    cust_city varchar(50) null,
    cust_state varchar(5) null,
    cust_zip varchar(8) null,
    cust_country varchar(50) null,
    cust_contact varchar(100) null,
    cust_email varchar(255) null,
    primary key(cust_id)
)engine = InnoDb auto_increment = 1000 default charset=utf8;//指定自增的开始节点



//防止表已经存在用if not exists tableName 并且这里创建了外键

create table  if not exists orders(
    order_num int not null auto_increment,
    order_date datetime not null,
    cust_id int not null,
    primary key(order_num),
    foreign key (cust_id) references customers(cust_id)
)engine = InnoDb default charset= utf8



create table  if not exists orders(
            order_num int not null auto_increment,
            order_date datetime not null,
            cust_id int not null,
            primary key(order_num)
)engine = InnoDb default charset= utf8


alter table orders add constraint fk_orders foreign key (cust_id) references customers(cust_id)



insert into orders values (123,Now(),1)//这里如果customers中没有1的cust_id数据存在会报错


insert into customers(cust_name,cust_address,cust_city,
                        cust_state,cust_zip,cust_country,
                        cust_contact,cust_email) 
            values('123','123','123','123','123','123','123','123');
        
        
        
单句修改是
Alter tableName auto_increment = xxx;因为一张表只能存在一个
auto_increment所以不需要指定columnName Tip 如果设置的值比表中已有的自增值小就无效

Select last_insert_id()获取刚刚生成使用的的自增长的值
        
修改步长
SHOW VARIABLES LIKE 'AUTO_INC%';//查看配置
        
        
 设置
SET auto_increment_increment = 3 ;//步长修改为3
Tip这种情况下重启服务会导致设置重置
        
需要配置my.ini在服务端配置也就是mysqld下增加auto_increment_increment = 10当然其他配置也在这里
        
        
insert into orders(order_date,cust_id)values(Now(),1)//这里成功了

Tip datetime格式年月日不能省略
        
select date(order_date) from orders
select time(order_date) from orders
        
        
        
        
Tip也可以通过show variables 来看标准格式


create table if not exists vendors(
    vend_id int not null ,
    vend_name varchar(50) not null,
    vend_address varchar(50) null,
    vend_city varchar(50) null,
    vend_state varchar(50) null,
    vend_zip varchar(50) null,
    vend_country varchar(50) null
)engine =InnoDb default charset = utf8;

alter table vendors add primary key(vend_id)
alter table vendors modify vend_id int not null auto_increment
        
        
create table if not exists orderitems(
    order_num int not null,
    order_item int not null,
    prod_id varchar(10) not null,
    quantity int not null,
    item_price decimal(10,2) not null,
    primary key(order_num,order_item)
)engine = InnoDB;


alter table orderitems add constrainfk_orderitems  foreign key(order_num) referenceorders(order_num)


alter table orderitems modify quantity int not null default 1


create table if not exists productNote(
    note_id int not null auto_increment,
    prod_id int not null,
    note_date datetime not null,
    note_text text null,
    primary key(note_id),
    fulltext(note_text)
)engine = MYISAM comment = '产品注释表';

更新表alter

新增一列
Alter table tableName add columnName  dataType[ null 等]


新增索引
CREATE INDEX index_name ON table(columnLIst)
ALTER TABLE table_name ADD INDEX index_name(column_list)
create index pro_index on products(prod_name)
alter table vendors add index  pro_index (vend_name)


CREATE  unique INDEX index_name ON table(columnLIst)
ALTER TABLE table_name ADD INDEX index_name(column_list)
alter table customers add unique index  cust_uindex(cust_contact)
    
    
    
    
修改
alter table tablename modify columnname xxxxxxx；


删除一列
Alter table tableName drop column columnName;


删除索引
Drop index index_name on table_name
Alter table tableName drop primary key//删除主键
Alter table tableName drop foreign  key fk_name//删键


新增外键/主键
Alter table tableName add constraint 外键名字 foreigkey (columnName) references tableName(columnName)

Alter table tableName add primary key(columnName)



删除表//这种方法没法撤销。
Drop table tableName
drop table if exists productnote 

视图同理
drop view if exists productnote 


Rename 
Rename table tableNameOld to tableNameNew





<div id=c4><h2>DCL   data control  language</h2></div>

1. ./mysqladmin -u root -p oldpassword newpasswd(记住这个命令是在/usr/local/mysql/bin中外部命令)
2. SET PASSWORD FOR root=PASSWORD(’new password’);（对登录数据库后这种方式）
3. UPDATE user SET password=PASSWORD(”new password”) WHERE user=’root’; 　（对登录数据库后这种方式）


update user set user.authentication_string = password("xxxx") where user.User = "root" and Host = "localhost";

    
    
show databases;展示所有的库。

create database noteex;新建库

create user ex@localhost identified by '123456' 创建密码为123456的本地用户ex

grant all on noteex.* to ex@localhost;赋予ex用户noteex的所有权限

Flush privileges


use noteex;使用noteex库

select database();//查询当前使用的数据库名字

select user();//查询当前用户名


show tables; 展示该库的所有表

desc tableName/show columns from tableName;展示表的字段结构


docker run -d -p 5432:5432 --name postgresql  -v hostdb_data:/var/lib/postgresql/data\ -e POSTGRES_PASSWORD=xxxx postgres


docker run -d -p 5433:80 --name pgadmin4 -e PGADMIN_DEFAULT_EMAIL=test@1234.com -e PGADMIN_DEFAULT_PASSWORD=xxxx dpage/pgadmin4



create database xxx;

CREATE USER aaaaa WITH ENCRYPTED PASSWORD 'xxxxx'
GRANT ALL PRIVILEGES ON DATABASE xxx TO aaaa;


CREATE TABLE  if not exists test_table (
    user_id SERIAL PRIMARY KEY, -- SERIAL 类型会自动创建自增的唯一标识
    name VARCHAR(256) UNIQUE
);
