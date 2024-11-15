from pymodbus.client.sync import ModbusTcpClient # type: ignore
import mysql.connector
from time import sleep

# Kết nối đến Modbus TCP
modbus_client = ModbusTcpClient('', port=502)  # Địa chỉ IP và port của Modbus server
modbus_client.connect()

# Kết nối đến MySQL
mydb = mysql.connector.connect(
  host="localhost",
  user="root",
  password="password",
  database="modbus_data"
)
mycursor = mydb.cursor()

# Tạo bảng nếu chưa có
mycursor.execute("""
CREATE TABLE IF NOT EXISTS sensor_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    register_value INT
)
""")

# Hàm đọc dữ liệu từ Modbus và lưu vào MySQL
def read_and_store_data():
    # Đọc dữ liệu từ Modbus, ví dụ đọc 1 thanh ghi từ địa chỉ 0
    result = modbus_client.read_holding_registers(0, 1)
    if result.isError():
        print("Error reading Modbus data")
    else:
        register_value = result.registers[0]
        print(f"Received data: {register_value}")

        # Lưu dữ liệu vào MySQL
        sql = "INSERT INTO sensor_data (register_value) VALUES (%s)"
        val = (register_value,)
        mycursor.execute(sql, val)
        mydb.commit()
        print("Data inserted into MySQL")

# Đọc và lưu dữ liệu mỗi 10 giây
while True:
    read_and_store_data()
    sleep(10)

# Đóng kết nối khi kết thúc
modbus_client.close()
mydb.close()
