﻿using Extensions;
using System;
using System.Collections.Generic;

namespace ReactNative.UIManager
{
    abstract class ReactShadowNodeVisitor<T>
    {
        public T Visit(ReactShadowNode node)
        {
            if (node == null)
            {
                throw new ArgumentNullException(nameof(node));
            }

            var n = node.ChildCount;
            if (n == 0)
            {
                return Make(node, Array<T>.Empty);
            }
            else
            {
                var children = new List<T>(n);
                for (var i = 0; i < node.ChildCount; ++i)
                {
                    var child = node.GetChildAt(i);
                    children.Add(Visit(child));
                }

                return Make(node, children);
            }
        }

        protected abstract T Make(ReactShadowNode node, IList<T> children);
    }
}
