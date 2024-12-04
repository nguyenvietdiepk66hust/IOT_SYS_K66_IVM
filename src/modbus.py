# import mysql.connector
# from pymodbus.client import ModbusTcpClient
# import time

# # 1. Kết nối Modbus
# def connect_modbus(host=' ', port=502):
#     client = ModbusTcpClient(host, port)
#     if client.connect():
#         print("Kết nối Modbus thành công!")
#     else:
#         print("Kết nối Modbus thất bại.")
#     return client

# # 2. Đọc dữ liệu từ Modbus
# def read_modbus_data(client):
#     # Giả sử bạn đọc các thanh ghi 16-bit (holding registers) từ địa chỉ 0, số lượng 10
#     result = client.read_holding_registers(0, 10, unit=1)
#     if result.isError():
#         print("Lỗi khi đọc dữ liệu Modbus.")
#         return None
#     return result.registers  # Trả về danh sách các thanh ghi đọc được

# # 3. Kết nối MySQL
# def connect_mysql():
#     connection = mysql.connector.connect(
#         host='localhost',         # Địa chỉ của MySQL
#         user='root',              # Tên người dùng
#         password='',      # Mật khẩu
#         database='modbus_data'    # Tên cơ sở dữ liệu
#     )
#     return connection

# # 4. Lưu dữ liệu vào cơ sở dữ liệu MySQL
# def save_to_mysql(data, connection):
#     cursor = connection.cursor()
#     for value in data:
#         # Giả sử chúng ta lưu dữ liệu vào bảng "modbus_readings" có các cột "reading_value" và "timestamp"
#         cursor.execute("INSERT INTO modbus_readings (reading_value, timestamp) VALUES (%s, NOW())", (value,))
#     connection.commit()
#     print(f"Đã lưu {len(data)} giá trị vào cơ sở dữ liệu.")

# # 5. Chạy chương trình
# def main():
#     # Kết nối Modbus
#     modbus_client = connect_modbus()
#     if not modbus_client:
#         return

#     # Kết nối MySQL
#     mysql_connection = connect_mysql()

#     try:
#         while True:
#             # Đọc dữ liệu từ Modbus
#             data = read_modbus_data(modbus_client)
#             if data:
#                 # Lưu dữ liệu vào cơ sở dữ liệu
#                 save_to_mysql(data, mysql_connection)
#             # Chờ 5 giây trước khi đọc lại dữ liệu
#             time.sleep(5)

# #     except KeyboardInterrupt:
# #         print("Dừng chương trình...")
# #     finally:
# #         # Đóng kết nối khi kết thúc
# #         modbus_client.close()
# #         mysql_connection.close()

# # if __name__ == "__main__":
# #     main()
