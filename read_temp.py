#!/usr/bin/env python3
"""
AMS_HT 温度读取脚本
从 ESP8266 HTTP 服务器读取 AHT20/BMP280 传感器数据

用法:
    python read_temp.py                      使用上次保存的IP，默认自动刷新
    python read_temp.py 192.168.1.100        指定IP，默认自动刷新
    python read_temp.py --once               单次读取（使用上次IP）
    python read_temp.py 192.168.1.100 --once 单次读取
    python read_temp.py -l 3                 自动刷新，间隔3秒
    python read_temp.py --new-ip             强制输入新IP
"""

import socket
import sys
import time
import re
import os
import json


def read_sensor_data(ip, port=80, timeout=5):
    """发送HTTP GET请求并解析传感器数据"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        sock.connect((ip, port))
        
        # 发送GET请求 (STM32的ESP8266服务器不关心请求行内容，只要有GET就响应)
        request = f"GET / HTTP/1.1\r\nHost: {ip}\r\nConnection: close\r\n\r\n"
        sock.sendall(request.encode())
        
        # 读取响应
        response = b""
        while True:
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
            except socket.timeout:
                break
        
        sock.close()
        
        if not response:
            return None, "无响应数据"
        
        # 解码响应，跳过HTTP头
        text = response.decode('utf-8', errors='replace')
        
        # 解析传感器数据
        data = {}
        
        # 匹配 AHT20: XX.X C, XX.X %RH
        aht_match = re.search(r'AHT20:\s*([\d.]+)\s*C,\s*([\d.]+)\s*%RH', text)
        if aht_match:
            data['aht_temp'] = float(aht_match.group(1))
            data['aht_hum'] = float(aht_match.group(2))
        
        # 匹配 BMP280: XX.X C, XXX kPa
        bmp_match = re.search(r'BMP280:\s*([\d.]+)\s*C,\s*(\d+)\s*kPa', text)
        if bmp_match:
            data['bmp_temp'] = float(bmp_match.group(1))
            data['bmp_pres'] = int(bmp_match.group(2))
        
        # 匹配 Countdown: HH:MM:SS
        cd_match = re.search(r'Countdown:\s*(\d+):(\d+):(\d+)', text)
        if cd_match:
            data['countdown'] = f"{cd_match.group(1)}:{cd_match.group(2)}:{cd_match.group(3)}"
            data['countdown_sec'] = int(cd_match.group(1))*3600 + int(cd_match.group(2))*60 + int(cd_match.group(3))
        
        # 匹配 Temp Threshold: XX-XX C
        th_match = re.search(r'Temp Threshold:\s*(\d+)-(\d+)\s*C', text)
        if th_match:
            data['threshold_low'] = int(th_match.group(1))
            data['threshold_high'] = int(th_match.group(2))
        
        # 匹配 Heating: ON/OFF
        heat_match = re.search(r'Heating:\s*(ON|OFF)', text)
        if heat_match:
            data['heating'] = heat_match.group(1) == 'ON'
        
        return data, None
        
    except socket.timeout:
        return None, f"连接超时 ({ip}:{port})"
    except ConnectionRefusedError:
        return None, f"连接被拒绝 ({ip}:{port})"
    except socket.gaierror:
        return None, f"无法解析地址: {ip}"
    except Exception as e:
        return None, f"错误: {e}"


def print_data(data):
    """格式化打印传感器数据"""
    print("\n" + "=" * 50)
    print(f"  {'AMS_HT 传感器数据':^46}")
    print("=" * 50)
    
    if 'aht_temp' in data:
        print(f"  AHT20  温度: {data['aht_temp']:>6.1f} °C    湿度: {data['aht_hum']:>5.1f} %RH")
    
    if 'bmp_temp' in data:
        print(f"  BMP280 温度: {data['bmp_temp']:>6.1f} °C    气压: {data['bmp_pres']:>5d} kPa")
    
    if 'threshold_low' in data:
        print(f"  温度阈值:  {data['threshold_low']} - {data['threshold_high']} °C")
    
    if 'countdown' in data:
        cd = data['countdown']
        sec = data.get('countdown_sec', 0)
        if sec > 0:
            print(f"  倒计时:    {cd}  (剩余 {sec} 秒)")
        else:
            print(f"  倒计时:    {cd}  (已结束)")
    
    if 'heating' in data:
        status = "● 加热中" if data['heating'] else "○ 已停止"
        print(f"  加热状态:  {status}")
    
    print("=" * 50)


CONFIG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '.ams_ht_config.json')


def load_config():
    """加载配置文件，返回 {'ip': 'xxx', 'interval': 5}"""
    try:
        if os.path.exists(CONFIG_FILE):
            with open(CONFIG_FILE, 'r') as f:
                return json.load(f)
    except Exception:
        pass
    return {}


def save_config(ip, interval=5):
    """保存配置到文件"""
    try:
        config = {'ip': ip, 'interval': interval}
        with open(CONFIG_FILE, 'w') as f:
            json.dump(config, f)
    except Exception:
        pass


def parse_args():
    """解析命令行参数，返回 (ip, loop_mode, interval, force_new_ip)"""
    raw_args = sys.argv[1:]
    
    loop_mode = '--loop' in raw_args or '-l' in raw_args
    once_mode = '--once' in raw_args
    force_new_ip = '--new-ip' in raw_args or '-n' in raw_args
    
    if once_mode:
        loop_mode = False
    
    # 找间隔参数
    interval = 5
    for i, arg in enumerate(raw_args):
        if arg in ('--loop', '-l'):
            if i + 1 < len(raw_args):
                try:
                    interval = int(raw_args[i + 1])
                except ValueError:
                    pass
    
    # 第一个非选项参数作为IP（排除纯数字间隔参数）
    ip = None
    for a in sys.argv[1:]:
        if a.startswith('-'):
            continue
        try:
            int(a)
        except ValueError:
            ip = a
            break
    
    return ip, loop_mode, interval, force_new_ip


def main():
    ip, loop_mode, interval, force_new_ip = parse_args()
    config = load_config()
    
    if force_new_ip:
        config = {}  # 强制重新输入，清除已保存的IP
    
    # 如果没有提供IP，优先使用配置文件中的IP
    if not ip:
        saved_ip = config.get('ip', '')
        saved_interval = config.get('interval', 5)
        
        if saved_ip and not force_new_ip:
            ip = saved_ip
            interval = saved_interval
        else:
            print("=" * 50)
            print(f"  {'AMS_HT 温度读取工具':^46}")
            print("=" * 50)
            print()
            if saved_ip in ('', None) or force_new_ip:
                pass  # 正常获取新IP
            while True:
                ip = input("请输入 ESP8266 IP 地址: ").strip()
                if ip:
                    break
                print("IP 地址不能为空，请重新输入。")
    
    # 默认自动刷新模式（除非指定 --once）
    if not loop_mode and '--once' not in sys.argv:
        loop_mode = True
    
    once_mode = '--once' in sys.argv
    
    if once_mode:
        # 单次模式：直接读取
        print(f"连接 ESP8266: {ip}:80 ...")
        data, error = read_sensor_data(ip)
        if error:
            print(f"读取失败: {error}")
            print()
            input("按回车键退出...")
            sys.exit(1)
        print_data(data)
        print()
        input("按回车键退出...")
        return
    
    # 自动刷新模式（默认）
    save_config(ip, interval)
    print(f"\n连接 ESP8266: {ip}:80 ...")
    print(f"自动刷新模式 (间隔 {interval} 秒), Ctrl+C 退出")
    print("提示: 使用 --once 参数可单次读取")
    print()
    try:
        while True:
            data, error = read_sensor_data(ip)
            if error:
                print(f"\r[{time.strftime('%H:%M:%S')}] {error}", end='')
            else:
                print_data(data)
            time.sleep(interval)
    except KeyboardInterrupt:
        print("\n已退出.")
        input("按回车键关闭...")


if __name__ == '__main__':
    main()