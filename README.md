## Monice : Monitor task nice and count folios per NUMA nodes

- Monice is kernel module for monitor task's nice and scan all folios
- This is for check nice value and scan all folios per task, but you can use just for folios per task

```
[usage]
1. make
2. insmod monice.ko
## if you want disable
3. rmmod monice
```

- You can see all monitoring result via using `dmesg`
