require "mysql"
require "print_r"

local db, h = mysql.connect('localhost', 'root', 'kernelx', 'testdb', 3306)
print(db)
print(h)
print('------------------------')
local db, h = mysql.connect('localhost', 'root', 'kernel', 'testdb')
--local db, err = mysql.connect('localhost:3308', 'root', 'kernel')
--local db, h = mysql.connect('localhost', 'root', 'kernel', 'testdb', 3306)
print(db)
print(err)

print('------------------------')
local db, err = mysql.connect('localhost', 'root', 'kernel')
print(db:select_db('testdb'))
--print(db:select_db('testdb3'))

print(db:query("insert `table` (`hits`,`time`,`col1`,`col2`) values (10000, 33333, 'haha', 'hehe')"))
print(db:insert_id())
local rs = db:query("select * from `table`")
local row = rs:fetch_array({}, 'a')
print_r(row)

print(rs:free_result())
print(db)
db:close()
print(db)
print(err)
--db:close()
--mysql.query(db, 'delete from `testdb`.`table` where `testdb`.`table`.`id` > 2')
