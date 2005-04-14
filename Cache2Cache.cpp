//--------------------
// Cache-2-Cache ping-pong latency measurement for Win32
// Compiled with Visual C++

#include <stdio.h>
#include <windows.h>


struct pingPongPtrs_t {
  volatile int *pPing;
  volatile int *pPong;
};
const int NITER = 100000000;
DWORD WINAPI PingPongThreadProc(void *pingPongPtrs)
{
  volatile int *pPing = ((pingPongPtrs_t *)pingPongPtrs)->pPing;
  volatile int *pPong = ((pingPongPtrs_t *)pingPongPtrs)->pPong;

  *pPong = 0;
  for (int i = 0; i < NITER; ++i)
  {
      while (*pPing < i);
      *pPong = i+1;
  }
  return 0;
}

int main()
{
  SYSTEM_INFO si; GetSystemInfo(&si);
  if (si.dwNumberOfProcessors > 1)
  {
      for (DWORD p1 = 0; p1 < si.dwNumberOfProcessors-1; ++p1)
        for (DWORD p2 = p1+1; p2 < si.dwNumberOfProcessors; ++p2)
        {
            printf("CPU%d<->CPU%d:", p1, p2);
            DWORD tid[2];
            int ping[32]; // place ping and pong in different cache lines
            int pong[32];
            pingPongPtrs_t prm1 = {ping, pong};
            pingPongPtrs_t prm2 = {pong, ping};
            HANDLE h1 = CreateThread(NULL, 0, PingPongThreadProc,
              &prm1, CREATE_SUSPENDED, &tid[0]);
            HANDLE h2 = CreateThread(NULL, 0, PingPongThreadProc,
              &prm2, CREATE_SUSPENDED, &tid[1]);
            SetThreadAffinityMask(h1, 1 << p1);
            SetThreadAffinityMask(h2, 1 << p2);
            ping[0] = pong[0] = 0; // start in known state
            ResumeThread(h1);
            Sleep(10);  // Improve consistency of the time measurement
            LARGE_INTEGER t0; QueryPerformanceCounter(&t0);
            ResumeThread(h2);
            WaitForSingleObject(h1, INFINITE);
            LARGE_INTEGER t1; QueryPerformanceCounter(&t1);
            // Cleanup
            WaitForSingleObject(h2, INFINITE);
            CloseHandle(h1);
            CloseHandle(h2);
            // Report result
            __int64 dt = t1.QuadPart - t0.QuadPart;
            LARGE_INTEGER fr; QueryPerformanceFrequency(&fr);
            double tmNs = ((dt * 1E9)/(fr.QuadPart))/NITER;
            printf(" %10.1fnS per ping-pong\n", tmNs);
        }
  }
  else
  {
      printf("The test can't run on single-CPU machine. Sorry.\n");
  }
  return 0;
}
//-------------------- The end