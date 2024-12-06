# import mysql.connector
# from pymodbus.client import ModbusTcpClient
# import time

# Kết nối Modbus
def connect_modbus():
    client = ModbusTcpClient('192.168.0.150')  # Địa chỉ IP của thiết bị Modbus
    if client.connect():
        print("Kết nối Modbus thành công!")
        return client
    else:
        print("Không thể kết nối đến Modbus.")
        return None

# Kết nối MySQL
def connect_mysql():
    try:
        connection = mysql.connector.connect(
            host='localhost',
            user='root',         # Tên người dùng MySQL
            password='', # Mật khẩu người dùng MySQL
            database='modbus_data' # Tên cơ sở dữ liệu
        )
        print("Kết nối MySQL thành công!")
        return connection
    except mysql.connector.Error as err:
        print(f"Lỗi kết nối MySQL: {err}")
        return None

# Đọc dữ liệu từ Modbus (giả sử bạn muốn đọc 2 thanh ghi từ địa chỉ 0)
def read_modbus_data(client):
    try:
        result = client.read_holding_registers(0, 2)  # Đọc 2 thanh ghi từ địa chỉ 0
        if result.isError():
            print("Lỗi khi đọc dữ liệu từ Modbus")
            return None
        else:
            # Lấy giá trị của 2 thanh ghi
            register1 = result.registers[0]
            register2 = result.registers[1]
            return register1, register2
    except Exception as e:
        print(f"Lỗi trong quá trình đọc Modbus: {e}")
        return None

# Lưu dữ liệu vào MySQL
def save_to_mysql(register1, register2, connection):
    try:
        cursor = connection.cursor()
        # Thực hiện truy vấn SQL để lưu dữ liệu
        query = "INSERT INTO modbus_data (register1, register2, timestamp) VALUES (%s, %s, NOW())"
        cursor.execute(query, (register1, register2))
        connection.commit()  # Lưu thay đổi vào cơ sở dữ liệu
        print(f"Đã lưu dữ liệu: Register1 = {register1}, Register2 = {register2}")
    except mysql.connector.Error as err:
        print(f"Lỗi khi lưu vào MySQL: {err}")

# # 5. Chạy chương trình
# def main():
#     # Kết nối Modbus
#     modbus_client = connect_modbus()
#     if not modbus_client:
#         return

    # Kết nối MySQL
    mysql_connection = connect_mysql()
    if not mysql_connection:
        return

    try:
        while True:
            # Đọc dữ liệu từ Modbus
            data = read_modbus_data(modbus_client)
            if data:
                register1, register2 = data
                # Lưu dữ liệu vào MySQL
                save_to_mysql(register1, register2, mysql_connection)
            
            # Chờ 5 giây trước khi đọc lại dữ liệu
            time.sleep(5)

# #     except KeyboardInterrupt:
# #         print("Dừng chương trình...")
# #     finally:
# #         # Đóng kết nối khi kết thúc
# #         modbus_client.close()
# #         mysql_connection.close()

# # if __name__ == "__main__":
# #     main()
