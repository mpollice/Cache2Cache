/*
Cache2Cache
Copyright (c) 2005 Michael S

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//--------------------
// Cache-2-Cache ping-pong latency measurement for Win32
// Compiled with Visual C++

#include <intrin.h>
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

DWORD WINAPI PingPongThreadProcNop(void *pingPongPtrs)
{
  volatile int *pPing = ((pingPongPtrs_t *)pingPongPtrs)->pPing;
  volatile int *pPong = ((pingPongPtrs_t *)pingPongPtrs)->pPong;

  *pPong = 0;
  for (int i = 0; i < NITER; ++i)
  {
      while (*pPing < i)
		  __nop();
      *pPong = i+1;
  }
  return 0;
}

DWORD WINAPI PingPongThreadProcPause(void *pingPongPtrs)
{
  volatile int *pPing = ((pingPongPtrs_t *)pingPongPtrs)->pPing;
  volatile int *pPong = ((pingPongPtrs_t *)pingPongPtrs)->pPong;

  *pPong = 0;
  for (int i = 0; i < NITER; ++i)
  {
      while (*pPing < i)
		  _mm_pause();
      *pPong = i+1;
  }
  return 0;
}

int main(int argc, const char* argv[])
{
  DWORD (_stdcall *pPingPongThreadProc)(void *pingPongPtrs) = PingPongThreadProc;
  if (argc > 1 && (strncmp(argv[1], "nop", 3) == 0))
  {
    pPingPongThreadProc = PingPongThreadProcNop;
	printf("Executing NOP in the loop.\n");
  }

  if (argc > 1 && (strncmp(argv[1], "pause", 5) == 0))
  {
    pPingPongThreadProc = PingPongThreadProcPause;
	printf("Executing PAUSE in the loop.\n");
  }

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
            HANDLE h1 = CreateThread(NULL, 0, *pPingPongThreadProc,
              &prm1, CREATE_SUSPENDED, &tid[0]);
            HANDLE h2 = CreateThread(NULL, 0, *pPingPongThreadProc,
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