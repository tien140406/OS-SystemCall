#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

static void
get_sysinfo(struct sysinfo *out)
{
  if (sysinfo(out) < 0) {
    printf("FAILED: sysinfo() returned error\n");
    exit(1);
  }
}

// Đo bộ nhớ trống thực tế bằng cách cấp từng trang qua sbrk() cho đến khi
// kernel từ chối, rồi giải phóng tất cả và trả về tổng số byte đã cấp được.

static uint64
measure_free_pages(void)
{
  char *base = sbrk(0); // trả về dịa chỉ base hiện tại heap
  uint64 total = 0;
  struct sysinfo st;

  // Cấp từng trang cho đến khi bị từ chối 
  while (sbrk(PGSIZE) != (char *)0xffffffffffffffff)
    total += PGSIZE;

  get_sysinfo(&st);
  if (st.freemem != 0) {
    printf("FAILED: memory exhausted but freemem = %ld\n", st.freemem); // vẫn còn RAM tức sai logic -> báo lỗi 
    exit(1);
  }

  // trả lại bộ nhớ đã xin
  // dấu trừ -> thu hồi 
  sbrk(-(sbrk(0) - base)); 
  return total;
}

// Check: freemem đúng khi cấp và giải phóng một trang bộ nhớ 
// sbrk(): xin thêm bộ nhớ từ hđh 
static void
check_freemem(void)
{
  struct sysinfo st;
  uint64 avail = measure_free_pages();

  // Before
  get_sysinfo(&st);
  printf("[freemem] baseline          = %ld bytes\n", st.freemem);
  if (st.freemem != avail) {
    printf("FAILED: freemem=%ld, want %ld\n", st.freemem, avail);
    exit(1);
  }

  // Calling 
  sbrk(PGSIZE);

  // After 
  get_sysinfo(&st);
  printf("[freemem] after alloc        = %ld bytes (expected %ld)\n", st.freemem, avail - PGSIZE);
  if (st.freemem != avail - PGSIZE) {
    printf("FAILED: freemem=%ld, want %ld\n", st.freemem, avail - PGSIZE); // sai logic
    exit(1);
  }

  // Giải phóng trang vừa cấp
  sbrk(-PGSIZE);
  
  // After
  get_sysinfo(&st);
  printf("[freemem] after free         = %ld bytes (expected %ld)\n", st.freemem, avail);
  if (st.freemem != avail) { 
    printf("FAILED: freemem=%ld, want %ld\n", st.freemem, avail); // sai logic
    exit(1);
  }

  printf("check_freemem PASSED\n\n");
}

// Kiểm tra: nproc đúng số tiến trình đang chạy qua fork/wait 
static void
check_nproc(void)
{
  struct sysinfo st;
  int wstatus;

  get_sysinfo(&st);

  // BEFORE 
  uint64 init_count = st.nproc;
  printf("[nproc] baseline             = %ld\n", init_count);


  int child = fork();
  if (child < 0) {
    printf("FAILED: fork() failed\n");
    exit(1);
  }

  if (child == 0) {
    // Tiến trình con: bảng tiến trình phải tăng đúng 1
    struct sysinfo cst;
    get_sysinfo(&cst);
    printf("[nproc] inside child         = %ld (expected %ld)\n", cst.nproc, init_count + 1);
    if (cst.nproc != init_count + 1) {
      printf("FAILED: child sees nproc=%ld, want %ld\n", cst.nproc, init_count + 1);
      exit(1);
    }
    exit(0);
  }

  // wait
  wait(&wstatus);

  // Tiến trình cha: sau khi con kết thúc, nproc phải trở về ban đầu 
  get_sysinfo(&st);
  printf("[nproc] after child exited   = %ld (expected %ld)\n", st.nproc, init_count);
  if (st.nproc != init_count) {
    printf("FAILED: nproc=%ld, want %ld\n", st.nproc, init_count);
    exit(1);
  }

  printf("check_nproc PASSED\n\n");
}

// Check: nopenfiles phản ánh đúng số file descriptor khi open/close 
static void
check_openfiles(void)
{
  struct sysinfo st;
  get_sysinfo(&st);

  // BEFORE
  uint64 init_fds = st.nopenfiles;
  printf("[openfiles] baseline         = %ld\n", init_fds);

  // open 
  int fd = open("README", 0);
  if (fd < 0) {
    printf("FAILED: open(\"README\") failed\n");
    exit(1);
  }

  // AFTER OPEN
  get_sysinfo(&st);
  printf("[openfiles] after open       = %ld (expected %ld)\n", st.nopenfiles, init_fds + 1);
  if (st.nopenfiles != init_fds + 1) {
    printf("FAILED: nopenfiles=%ld, want %ld\n", st.nopenfiles, init_fds + 1);
    exit(1);
  }

  // close file đó lại 
  close(fd);

  // AFTER CLOSE
  get_sysinfo(&st);
  printf("[openfiles] after close      = %ld (expected %ld)\n", st.nopenfiles, init_fds);
  if (st.nopenfiles != init_fds) {
    printf("FAILED: nopenfiles=%ld, want %ld\n", st.nopenfiles, init_fds);
    exit(1);
  }

  printf("check_openfiles PASSED\n\n");
}

int
main(void)
{
  // TEST
  printf("=== sysinfotest start ===\n\n");
  check_freemem();
  check_nproc();
  check_openfiles();
  printf("=== sysinfotest OK ===\n");

  // PRINT OUT CURRENT
  struct sysinfo st;
  get_sysinfo(&st);
  printf("[sysinfo]: freemem=%ld, nproc=%ld, nopenfiles=%ld\n", st.freemem, st.nproc, st.nopenfiles);
  exit(0);
}
