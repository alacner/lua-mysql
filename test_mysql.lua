require "mysql"
require "print_r"

local db, h,d = mysql.connect('localhost', 'root', 'kernelx', 'testdb', 3306)
print(db)
print(h)
print(d)
print('-----#1-------------------')
local db, err, a = mysql.connect('example.com:3307', 'root', 'kernel')
--local db, h = mysql.connect('localhost', 'root', 'kernel', 'testdb', 3306)
print(db)
print(err)
print(a)
print('-----#2-------------------')
local db, err, a = mysql.connect('localhost:/tmp/mysql.sock', 'root', 'kernel')
--local db, h = mysql.connect('localhost', 'root', 'kernel', 'testdb', 3306)
print(db)
print(err)
print(a)
os.exit();
print('------------------------')
local db, err = mysql.connect('localhost', 'root', 'kernel')
print(db:select_db('testdb'))
--print(db:select_db('testdb3'))

print(db:set_charset("utf8")) -- there is no char '-'
print(db:query("insert `table` (`hits`,`time`,`col1`,`col2`) values (10000, 33333, '天使', 'hehe')"))
print(db:insert_id())
print(db:affected_rows())
print(db:query("insert `table` (`dhits`,`time`,`col1`,`col2`) values (10000, 33333, '天使', 'hehe')"))
print(db:affected_rows())
print(db:escape_string('哈哈"'))
print(db:real_escape_string([['哈哈`"'\n]]))
--os.exit();
local rs = db:query("select * from `table` limit 30")
--local rs = db:unbuffered_query("select * from `table`")
--print(rs:num_fields())
--print(rs:num_rows())
--os.exit();
print('----row------------')
--[[
local row = rs:fetch({}, 'a')
while row do
    print_r(row)
    row = rs:fetch({}, 'a')
end
--]]
--print_r(row)
--print('------array mysql_num----------')
--local row = rs:fetch_array("MYSQL_NUM")
--print_r(row)
--print('--------assoc--------')
--local row = rs:fetch_assoc()
--print_r(row)
--os.exit();
local row = rs:fetch_row()
while row do
    print_r(row)
    row = rs:fetch_row()
end
--[[
while row do
    print_r(row)
    row = rs:fetch_assoc()
end
--]]
print('===================')
os.exit();
local row = rs:fetch_array({}, 'a')
while row do
    print_r(row)
    row = rs:fetch_array({}, 'a')
end

local row = rs:fetch_array({}, 'n')
print_r(row)

print(rs:free_result())
print(db)
db:close()
print(db)
print(err)
--db:close()
--mysql.query(db, 'delete from `testdb`.`table` where `testdb`.`table`.`id` > 2')

--[==[
local db, err = mysql.connect('localhost', 'root', 'kernel')
print(db:select_db('testdb'))
while true do
    print(db:query("insert `table` (`hits`,`time`,`col1`,`col2`) values (10000, 33333, 'haha', 'hehe')"))
    print(db:insert_id())
--[[
    local rs = db:query("select * from `table`")
    local row = rs:fetch_array({}, 'a')
    while row do
        print_r(row)
        row = rs:fetch_array({}, 'a')
    end

    --local row = rs:fetch_array({}, 'n')
    --print_r(row)
    rs:free_result()
--]]
end

db:close()
--]==]
