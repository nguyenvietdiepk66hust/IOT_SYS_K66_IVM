import time
from pymodbus.client import ModbusTcpClient
import mysql.connector

# Kết nối Modbus
def connect_modbus():
    client = ModbusTcpClient('192.168.0.130')  # Địa chỉ IP của thiết bị Modbus
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
            password='Nambeo12!',         # Mật khẩu người dùng MySQL
            database='data'      # Tên cơ sở dữ liệu
        )
        print("Kết nối MySQL thành công!")
        return connection
    except mysql.connector.Error as err:
        print(f"Lỗi kết nối MySQL: {err}")
        return None

# Đọc dữ liệu từ Modbus (64 thanh ghi)
def read_modbus_data(client):
    try:
        result = client.read_holding_registers(0, 64)  # Đọc 64 thanh ghi từ địa chỉ 0
        if result.isError():
            print("Lỗi khi đọc dữ liệu từ Modbus")
            return None
        else:
            # Lấy giá trị của 64 thanh ghi
            return result.registers
    except Exception as e:
        print(f"Lỗi trong quá trình đọc Modbus: {e}")
        return None

# Lưu dữ liệu vào MySQL cho mỗi bảng slave (slave1 đến slave16)
def save_to_mysql(registers_group, slave_number, connection):
    try:
        cursor = connection.cursor()
        
        # Xác định tên bảng tương ứng với slave_number
        table_name = f"slave{slave_number}"
        
        # Thực hiện truy vấn SQL để lưu dữ liệu vào bảng tương ứng
        query = f"INSERT INTO {table_name} (x, y, z, temperature, time) VALUES (%s, %s, %s, %s, NOW())"
        cursor.execute(query, registers_group)
        connection.commit()  # Lưu thay đổi vào cơ sở dữ liệu
        print(f"Đã lưu dữ liệu vào {table_name}: {registers_group}")
    except mysql.connector.Error as err:
        print(f"Lỗi khi lưu vào MySQL: {err}")

# Chạy chương trình chính
def main():
    # Kết nối Modbus
    modbus_client = connect_modbus()
    if not modbus_client:
        return

    # Kết nối MySQL
    mysql_connection = connect_mysql()
    if not mysql_connection:
        return

    try:
        while True:
            # Đọc dữ liệu từ Modbus (64 thanh ghi)
            registers = read_modbus_data(modbus_client)
            if registers:
                # Phân phối các giá trị vào các bảng slave
                for i in range(0, len(registers), 4):
                    registers_group = registers[i:i+4]
                    
                    # Tính toán bảng slave tương ứng (slave1 đến slave16)
                    slave_number = (i // 4) + 1
                    
                    # Lưu dữ liệu vào MySQL cho bảng tương ứng
                    save_to_mysql(registers_group, slave_number, mysql_connection)
            
            # Chờ 5 giây trước khi đọc lại dữ liệu
            time.sleep(5)

    except KeyboardInterrupt:
        print("Dừng chương trình...")
    finally:
        # Đóng kết nối khi kết thúc
        modbus_client.close()
        mysql_connection.close()

if __name__ == "__main__":
    main()
