using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Transition_Effect
{
    public class Effect
    {
        public string Name { get; set; }
        public AdvancedMediaSource.TransitionEffectType EffectType { get; set; }
    }

    public class TransitionEffect
    {
        public string Name { get; set; }
        public TransitionEffectTransform.TransitionEffectType EffectType { get; set; }
    }
}
