using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;

namespace ReactNative.Bridge
{
    public static class DispatcherHelpers
    {
        private static Dispatcher s_mainDispatcher;

        public static Dispatcher MainDispatcher
        {
            get
            {
                AssertDispatcherSet();

                return s_mainDispatcher;
            }

            internal set
            {
                if (value == null)
                {
                    throw new ArgumentNullException(nameof(value));
                }

                if (value.Thread.GetApartmentState() != ApartmentState.STA)
                {
                    throw new ArgumentException("Dispatcher must be an STA thread");
                }

                s_mainDispatcher = value;
            }
        }

        /// <summary>
        /// Sets the main dispatcher.
        /// </summary>
        public static void Initialize()
        {
            s_mainDispatcher = Dispatcher.CurrentDispatcher;
        }

        public static bool IsDispatcherSet()
        {
            return s_mainDispatcher != null;
        }

        public static void AssertOnDispatcher()
        {
            if (!IsOnDispatcher())
            {
                throw new InvalidOperationException("Thread does not have dispatcher access.");
            }
        }

        public static bool IsOnDispatcher()
        {
            AssertDispatcherSet();

            return MainDispatcher.CheckAccess();
        }

        public static void AssertOnDispatcher(DependencyObject dependencyObject)
        {
            AssertDispatcherSet();

            if (dependencyObject.Dispatcher != s_mainDispatcher)
            {
                throw new InvalidOperationException("Thread does not have dispatcher access (only main dispatcher is supported).");
            }
        }

        public static async void RunOnDispatcher(Action action)
        {
            AssertDispatcherSet();

            await MainDispatcher.InvokeAsync(action).Task.ConfigureAwait(false);
        }

        public static async void RunOnDispatcher(DispatcherPriority priority, Action action)
        {
            AssertDispatcherSet();

            await MainDispatcher.InvokeAsync(action, priority).Task.ConfigureAwait(false);
        }
        public static async void RunOnDispatcher(Dispatcher dispatcher, DispatcherPriority priority, Action action)
        {
            AssertDispatcherSet();

            if (dispatcher != s_mainDispatcher)
            {
                throw new InvalidOperationException("Only main dispatcher is supported.");
            }

            RunOnDispatcher(priority, action);
        }

        public static Task<T> CallOnDispatcher<T>(Func<T> func)
        {
            var taskCompletionSource = new TaskCompletionSource<T>();

            RunOnDispatcher(() =>
            {
                var result = func();

                // TaskCompletionSource<T>.SetResult can call continuations
                // on the awaiter of the task completion source.
                Task.Run(() => taskCompletionSource.SetResult(result));
            });

            return taskCompletionSource.Task;
        }

        /// <summary>
        /// Invokes a function on the dispatcher and asynchronously returns the
        /// result. Tries to inline the call if thread has dispatcher access.
        /// </summary>
        /// <typeparam name="T">Function return type.</typeparam>
        /// <param name="func">The function to invoke.</param>
        /// <returns>A task to await the result.</returns>
        public static Task<T> CallOnDispatcherWithInlining<T>(Func<T> func)
        {
            if (MainDispatcher.CheckAccess())
            {
                return Task.FromResult(func());
            }
            else
            {
                return CallOnDispatcher(func);
            }
        }

        public static void Reset()
        {
            // No-op on WPF
        }

        private static void AssertDispatcherSet()
        {
            if (s_mainDispatcher == null)
            {
                throw new InvalidOperationException("Dispatcher has not been set");
            }
        }
    }
}
